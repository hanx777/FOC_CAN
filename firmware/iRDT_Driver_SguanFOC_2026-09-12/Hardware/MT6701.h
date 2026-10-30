/*
 * @Author: 星必尘Sguan
 * @Date: 2026-07-14
 * @LastEditors: 星必尘Sguan|3464647102@qq.com
 * @LastEditTime: 2026-07-14
 * @FilePath: \demo_CANopenMOTOR\Hardware\MT6701.h
 * @Description: MT6701CT-STD-R 磁编码器驱动，使用DMA+SPI读取SSI接口数据
 *
 * 硬件连接:
 *   PB3 (SPI1_SCK)  → MT6701 PIN7 "B"  (10K上拉到5V)
 *   PB4 (SPI1_MISO) → MT6701 PIN6 "A"  (10K上拉到5V)
 *   PD2 (GPIO PP)   → MT6701 PIN8 "Z"  (CS片选, 10K上拉到5V)
 *   PA7 (SPI1_MOSI) → 悬空 (SPI全双工模式需要，未连接MT6701)
 *
 * Copyright (c) 2026 by $JUST, All Rights Reserved.
 */
#ifndef __MT6701_H
#define __MT6701_H

#include "main.h"

/* CS片选控制宏 (PD2) */
#define MT6701_CS_LOW()   GPIOD->BSRR = ((uint32_t)GPIO_PIN_2 << 16U)
#define MT6701_CS_HIGH()  GPIOD->BSRR = GPIO_PIN_2

/* MT6701分辨率参数 */
#define MT6701_RESOLUTION_BITS  (14U)
#define MT6701_COUNTER_MAX      (1U << MT6701_RESOLUTION_BITS)

/*
 * MT6701 SSI 24位数据帧 (MSB先收到):
 *   raw[23]     = 错误/起始标志位 (正常=1)
 *   raw[22:9]   = 14位角度数据
 *   raw[8:5]    = 4位磁场状态
 *   raw[5:0]    = 6位CRC校验值
 */

/* 从24位raw值提取各字段的掩码 */
#define MT6701_CRC_MASK         0x0000003FU   /* raw[5:0]   CRC6 */
#define MT6701_ANGLE_MASK       0x007FE000U   /* raw[22:9]  14位角度 */
#define MT6701_ANGLE_SHIFT      9U

/* 传感器状态 */
typedef struct {
    uint32_t data_raw;                  /* SPI 24位原始值 */
    uint32_t data_last;                 /* 上一次有效数据 */
    uint8_t  error_raw;                 /* 当前错误标志 (1=错误) */
    uint8_t  error_valid;               /* 滤波后的错误标志 */
    uint16_t error_filter_cnt;          /* 错误滤波计数器 */
    float    mech_angle;                /* 机械角度 [0, 2π] 弧度 */
} MT6701_SensorType;

/* DMA接收缓冲区 (供HAL_SPI_Receive_DMA使用) */
extern uint8_t mt6701_rx_buf[3];

/* 公有函数声明 */
void  MT6701_Init(void);
void  MT6701_TriggerRead(void);         /* 拉低CS + 启动DMA读取，由TIM1 ISR调用 */
void  MT6701_ProcessDMA(void);          /* DMA完成后处理数据，由SPI回调调用 */
float MT6701_GetAngle(void);

#endif /* __MT6701_H */
