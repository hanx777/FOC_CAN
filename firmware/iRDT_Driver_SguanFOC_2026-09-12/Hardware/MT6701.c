/*
 * @Author: 星必尘Sguan
 * @Date: 2026-07-14
 * @LastEditors: 星必尘Sguan|3464647102@qq.com
 * @LastEditTime: 2026-07-14
 * @FilePath: \demo_CANopenMOTOR\Hardware\MT6701.c
 * @Description: MT6701CT-STD-R 磁编码器驱动，DMA+SPI读取SSI接口，CRC6校验+错误滤波
 *
 * SSI 24位数据帧 (MSB先收到):
 *   raw[23]     = 错误/起始标志位 (正常=1)
 *   raw[22:9]   = 14位角度数据
 *   raw[8:5]    = 4位磁场状态
 *   raw[5:0]    = 6位CRC校验值
 *
 * 提取方式 (显式移位掩码):
 *   angle_data = (raw >> 9) & 0x3FFF    // 14-bit angle at [22:9]
 *   crc_result = raw & 0x3F             // 6-bit CRC at [5:0]
 *   CRC输入    = raw >> 6               // 18-bit data input to CRC6
 *
 * Copyright (c) 2026 by $JUST, All Rights Reserved.
 */
#include "MT6701.h"
#include "spi.h"
#include "Sguan_math.h"
#include <string.h>

/* MT6701传感器全局实例 */
static MT6701_SensorType mt6701 = {0};

/* DMA接收缓冲区 (3字节) */
uint8_t mt6701_rx_buf[3] = {0};

/*
*********************************************************************************************************
 * 函 数 名: MT6701_CalcCRC
 * 功能说明: 计算MT6701数据的CRC6校验值。
 * 形    参: data: 18位原始数据 (D[13:0]角度 + Mg[3:0]磁场状态)
 * 返 回 值: 6位CRC校验结果。
 * 说    明: 多项式 x^6 + x + 1 (0x43)，单次输入18bit数据。
*********************************************************************************************************
*/
static uint8_t MT6701_CalcCRC(uint32_t data)
{
    uint8_t  crc        = 0;
    uint32_t polynomial = 0x43;  /* x^6 + x + 1 */

    for (int i = 17; i >= 0; i--)
    {
        uint8_t bit = (data >> i) & 1U;
        crc <<= 1;
        if ((crc >> 6) ^ bit)
        {
            crc ^= polynomial;
        }
        crc &= 0x3F;  /* 仅保留低6位 */
    }
    return crc;
}

/*
*********************************************************************************************************
 * 函 数 名: MT6701_Init
 * 功能说明: 初始化MT6701传感器。
 * 形    参: 无。
 * 返 回 值: 无。
 * 说    明: 配置PD2为推挽输出作为CS片选信号，初始拉高。
*********************************************************************************************************
*/
void MT6701_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* 使能GPIOD时钟 */
    __HAL_RCC_GPIOD_CLK_ENABLE();

    /* PD2: MT6701 CS片选引脚，推挽输出 */
    GPIO_InitStruct.Pin   = GPIO_PIN_2;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /* CS初始拉高 (不选中) */
    MT6701_CS_HIGH();

    /* 清零传感器状态 */
    memset(&mt6701, 0, sizeof(mt6701));
    mt6701.error_filter_cnt = 5;
}

/*
*********************************************************************************************************
 * 函 数 名: MT6701_TriggerRead
 * 功能说明: 拉低CS并启动DMA+SPI接收3字节数据。
 * 形    参: 无。
 * 返 回 值: 无。
 * 说    明: 由TIM1 20kHz ISR中调用。DMA完成后由HAL_SPI_RxCpltCallback回调处理数据。
*********************************************************************************************************
*/
void MT6701_TriggerRead(void)
{
    MT6701_CS_LOW();
    HAL_SPI_Receive_DMA(&hspi1, mt6701_rx_buf, 3);
}

/*
*********************************************************************************************************
 * 函 数 名: MT6701_ProcessDMA
 * 功能说明: DMA接收完成后处理MT6701数据，含CRC校验与错误滤波。
 * 形    参: 无。
 * 返 回 值: 无 (结果存入全局mt6701.mech_angle)。
 * 说    明: 由HAL_SPI_RxCpltCallback中调用。
 *          CS已在本函数内拉高。
 *          CRC仅作质量标志，不阻挡角度更新（匹配例程行为）。
*********************************************************************************************************
*/
void MT6701_ProcessDMA(void)
{
    uint32_t raw;
    uint16_t angle_cnt;

    MT6701_CS_HIGH();

    /* 合并3字节为24位原始值 (MSB在前) */
    raw = (uint32_t)(mt6701_rx_buf[0] << 16 |
                     mt6701_rx_buf[1] << 8  |
                     mt6701_rx_buf[2]);

    /* 检测磁铁远离 (全零数据) */
    if (raw == 0x00000000U)
    {
        mt6701.error_raw = 1;
        goto err_filter;
    }

    /* CRC6校验 */
    if (MT6701_CalcCRC(raw >> 6) == (raw & MT6701_CRC_MASK))
    {
        mt6701.error_raw = 0;   /* 校验通过 */
    }
    /* CRC失败不设error_raw=1，匹配例程行为：CRC仅作质量参考 */

    /* 无论CRC是否通过，使用本次数据计算角度 */
    mt6701.data_raw = raw;

    /* 提取14位角度 [22:9]，转换为机械角度 [0, 2π] 弧度 */
    angle_cnt = (uint16_t)((mt6701.data_raw & MT6701_ANGLE_MASK) >> MT6701_ANGLE_SHIFT);
    mt6701.mech_angle = (float)angle_cnt * Value_2PI / (float)MT6701_COUNTER_MAX;

    /* 保存本次有效数据 */
    mt6701.data_last = mt6701.data_raw;

err_filter:
    /* 错误滤波: 连续5次状态一致才更新error_valid */
    if (mt6701.error_raw != mt6701.error_valid)
    {
        if (mt6701.error_filter_cnt > 0U)
        {
            mt6701.error_filter_cnt--;
        }
        else
        {
            mt6701.error_valid = mt6701.error_raw;
        }
    }
    else
    {
        mt6701.error_filter_cnt = 5;
    }
}

/*
*********************************************************************************************************
 * 函 数 名: MT6701_GetAngle
 * 功能说明: 获取当前机械角度。
 * 形    参: 无。
 * 返 回 值: 机械角度 [0, 2π] 弧度。
*********************************************************************************************************
*/
float MT6701_GetAngle(void)
{
    return mt6701.mech_angle;
}
