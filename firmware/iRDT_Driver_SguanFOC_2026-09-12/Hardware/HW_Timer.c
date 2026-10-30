/*
 * @Author: 星必尘Sguan
 * @Date: 2025-05-08 19:26:48
 * @LastEditors: 星必尘Sguan|3464647102@qq.com
 * @LastEditTime: 2026-02-21 01:14:10
 * @FilePath: \stm_SguanFOCtest\Hardware\Timer.c
 * @Description: TIM定时中断统一管理函数;
 * 
 * Copyright (c) 2025 by $JUST, All Rights Reserved. 
 */
#include "HW_Timer.h"
#include "adc.h"
#include "SguanFOC.h"
#include "main.h"
#include "MT6701.h"
#include "spi.h"

extern UART_HandleTypeDef huart3;
volatile uint32_t ADC_InjectedValues[3] = {0};
uint32_t taskCounter_100us = 0u;
extern void CANOpen_TIM_CallBack(void);
extern void bsp_KeyScan10ms(void);

/**
 * @description: TIM中断回调函数，1ms和100us的定时器定时中断;
 * @param {TIM_HandleTypeDef*} htim
 * @return {*}
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim == &htim1){  // 20kHz中断
        HAL_ADCEx_InjectedStart_IT(&hadc1);

        /* 触发MT6701 DMA+SPI读取 */
        MT6701_TriggerRead();
    }
    else if (htim->Instance == TIM2) //100us
    {
        CANOpen_TIM_CallBack();
        taskCounter_100us++;
        if(taskCounter_100us>100)//10ms
        {
            taskCounter_100us = 0;
            bsp_KeyScan10ms();
        }
        // SguanFOC状态机判断函数
        SguanFOC_Low_Loop();
    }
}

/**
 * @description: ADCz注入转换完成回调函数，20Khz的定时器定时中断;
 * @param {ADC_HandleTypeDef} *hadc
 * @return {*}
 */
void HAL_ADCEx_InjectedConvCpltCallback(ADC_HandleTypeDef *hadc){		
	ADC_InjectedValues[0] = ADC1->JDR1;             // 获取C相电流
	ADC_InjectedValues[1] = ADC1->JDR2;             // 获取B相电流
	ADC_InjectedValues[2] = ADC1->JDR3;             // 获取A相电流

    // SguanFOC电机控制实现函数
	SguanFOC_High_Loop();
}

/**
 * @description: UART接收完成回调函数，用于处理串口接收的数据;
 * @param {UART_HandleTypeDef} *huart
 * @param {uint16_t} Size
 * @return {*}
 */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
    if (huart->Instance == USART3) {
        // SguanFOC串口接收的解析函数
        // 直接调用Printf_ProcessData处理接收到的数据
        // 接受数据格式：XX=xx? (其中XX是指令，xx是浮点数)
        SguanFOC_Printf_Loop(Sguan_PrintfBuff, Size);

        // 重新启动DMA接收
        HAL_UARTEx_ReceiveToIdle_DMA(&huart3, Sguan_PrintfBuff, sizeof(Sguan_PrintfBuff));
        __HAL_DMA_DISABLE_IT(huart3.hdmarx, DMA_IT_HT);
    }
}

/**
 * @description: SPI1 DMA接收完成回调，处理MT6701数据;
 * @param {SPI_HandleTypeDef} *hspi
 * @return {*}
 */
void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi == &hspi1)
    {
        MT6701_ProcessDMA();    /* 拉高CS + 解析数据 + CRC校验 + 更新角度 */
    }
}

