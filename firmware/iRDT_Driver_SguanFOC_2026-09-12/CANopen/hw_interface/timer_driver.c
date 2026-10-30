/*
This file is part of CanFestival, a library implementing CanOpen Stack.

Copyright (C): Edouard TISSERANT and Francis DUPIN
AT91 Port: Peter CHRISTEN

See COPYING file for copyrights details.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

// Includes for the Canfestival driver
#include "canfestival.h"
#include "timer.h"
#include "tim.h"
//#include "bsp_flash.h"

/************************** Modul variables **********************************/
// Store the last timer value to calculate the elapsed time
TIMEVAL last_time_set = TIMEVAL_MAX;
TIMEVAL timer_base_cnt = 0U;
TIMEVAL timer_alarm_cnt = 1U;

/**
  * 函数功能: 设置定时器
  * 输入参数: value TIMEVAL (unsigned long)
  * 返 回 值: 无
  * 说    明: Set the timer for the next alarm.
  */
void setTimer(TIMEVAL value)
{
    timer_alarm_cnt += value;
    if(timer_alarm_cnt > TIMEVAL_MAX)
    {
        timer_alarm_cnt -= TIMEVAL_MAX;
    }
}
/**
  * 函数功能: 获取已消耗的时间
  * 输入参数: 无
  * 返 回 值: TIMEVAL (unsigned long)
  * 说    明: Return the elapsed time to tell the stack how much time is spent since last call.
  */
TIMEVAL getElapsedTime(void)
{
  return timer_base_cnt > last_time_set ? (timer_base_cnt - last_time_set) : (last_time_set - timer_base_cnt);
}

/**
  * 函数功能: 定时器Timer00,channel1中断处理函数
  * 输入参数: 无
  * 返 回 值: 无
  * 说    明: 无
  */
void CANOpen_TIM_CallBack(void)
{
    if(timer_base_cnt == timer_alarm_cnt)
    {
        last_time_set = timer_base_cnt;
        TimeDispatch();
    }
    timer_base_cnt ++;//add per 100us
    if(timer_base_cnt > TIMEVAL_MAX)
    {
        timer_base_cnt = 0u;
    }
}

/**
  * 函数功能: CANopen,Node-ID初始化
  * 输入参数: 无
  * 返 回 值: 无
  * 说    明: 无
  */
void initNodes(CO_Data* d, UNS32 id)
{
    /****************************** INITIALISATION SLAVE *******************************/
    /*set node id */
    setNodeId(&objDict_SingleAxis_Data, id);
    extern UNS32 objDict_SingleAxis_obj1400_COB_ID_used_by_PDO;
    objDict_SingleAxis_obj1400_COB_ID_used_by_PDO -= Canopen_NodeId;//RPDO1 used for all slave
    /* init */
    setState(&objDict_SingleAxis_Data, Pre_operational);   //若直接进入初始化,则NMT设置为Stop后,心跳包也停止了,Initialisation两次会出现2个节点上报报文
    setState(&objDict_SingleAxis_Data, Initialisation);    //boot-up该节点 进入Pre-optional状态,发送节点上报报文
    setState(&objDict_SingleAxis_Data, Operational);
}

/**
  * 函数功能: 开启定时器
  * 输入参数: 无
  * 返 回 值: 无
    * 说    明: 用于循环发送心跳包和TPDO
  */
void startTimerLoop(UNS32 id, TimerCallback_t _init_callback) 
{
    SetAlarm(NULL, id, _init_callback, 0, 0);//立刻跳转到InitNodes,初始化节点
}

/**
  * 函数功能: 启动初始化
  * 输入参数: 无
  * 返 回 值: 无
  * 说    明: CANopen协议起始的地方
  */
void CANopen_Init(void)
{
    //设置节点NODE_ID=0x03
    Canopen_NodeId = 0x03;
    //Canopen_NodeId = bsp_flash_read_canid();//读取flash中存储的节点ID
    if(Canopen_NodeId == 0xFF)
    {
        Canopen_NodeId = NODE_ID;
    }
    startTimerLoop(Canopen_NodeId, &initNodes);// Start timer
}
