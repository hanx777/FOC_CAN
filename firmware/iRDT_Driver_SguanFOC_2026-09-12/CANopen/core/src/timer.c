/*
This file is part of CanFestival, a library implementing CanOpen Stack.

Copyright (C): Edouard TISSERANT and Francis DUPIN

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
/*!
** @file   timer.c
** @author Edouard TISSERANT and Francis DUPIN
** @date   Tue Jun  5 09:32:32 2007
**
** @brief
**
**
*/

/* #define DEBUG_WAR_CONSOLE_ON */
/* #define DEBUG_ERR_CONSOLE_ON */

#include <applicfg.h>
#include "timer.h"

/*  ---------  The timer table --------- */
s_timer_entry timers[MAX_NB_TIMER] = {{TIMER_FREE, NULL, NULL, 0, 0, 0},};

TIMEVAL total_sleep_time = TIMEVAL_MAX;
TIMER_HANDLE last_timer_raw = -1;

#define min_val(a,b) ((a<b)?a:b)

/*!
** -------  Use this to declare a new alarm ------
**
** @param d
** @param id
** @param callback
** @param value
** @param period
**
** @return
**/
TIMER_HANDLE SetAlarm(CO_Data* d, UNS32 id, TimerCallback_t callback, TIMEVAL value, TIMEVAL period)
{
    TIMER_HANDLE row_number;
    s_timer_entry *row;
    /* 遍历所有定时器事件入口，查找空闲入口 */
    /* in order to decide new timer setting we have to run over all timer rows */
    for(row_number=0, row=timers; row_number <= last_timer_raw + 1 && row_number < MAX_NB_TIMER; row_number++, row++)
    {
        if (callback &&     /* if something to store */
           row->state == TIMER_FREE) /* and empty row */
        {   /* just store */
            TIMEVAL real_timer_value;
            TIMEVAL elapsed_time;
            
            /* 更新定时器事件在事件入口数组中的最大下标 */
            if (row_number == last_timer_raw + 1) last_timer_raw++;

            elapsed_time = getElapsedTime();
            /* set next wakeup alarm if new entry is sooner than others, or if it is alone */
            real_timer_value = value;
            real_timer_value = min_val(real_timer_value, TIMEVAL_MAX);
            
            /* 比较原本预定下一次唤醒时间和本事件触发时间，如果本事件先触发，则更新定时器唤醒时间 */
            if (total_sleep_time > elapsed_time && total_sleep_time - elapsed_time > real_timer_value)
            {
                /* 更新唤醒时间 */
                total_sleep_time = elapsed_time + real_timer_value;
                /* 设置定时器重载值 */
                setTimer(real_timer_value);
            }
            row->callback = callback;
            row->d = d;
            row->id = id;
            row->val = value + elapsed_time;
            row->interval = period;
            row->state = TIMER_ARMED;
            /* 返回定时器事件入口数组下标，作为句柄 */
            return row_number;
        }
    }

    return TIMER_NONE;
}

/*!
**  -----  Use this to remove an alarm ----
**
** @param handle
**
** @return
**/
TIMER_HANDLE DelAlarm(TIMER_HANDLE handle)
{
    /* Quick and dirty. system timer will continue to be trigged, but no action will be preformed. */
    MSG_WAR(0x3320, "DelAlarm. handle = ", handle);
    /* 将该定时事件入口状态置为空闲 */
    if(handle != TIMER_NONE)
    {
        if(handle == last_timer_raw)
            last_timer_raw--;
        timers[handle].state = TIMER_FREE;
    }
    return TIMER_NONE;
}

/*!
** ------  TimeDispatch is called on each timer expiration ----
**定时事件触发处理函数
**/
int tdcount=0;
void TimeDispatch(void)
{
    TIMER_HANDLE i;
    TIMEVAL next_wakeup = TIMEVAL_MAX; /* used to compute when should normaly occur next wakeup */
    /* First run : change timer state depending on time */
    /* Get time since timer signal 获取定时器中断到现在已经流逝的时间*/
    UNS32 overrun = (UNS32)getElapsedTime();
  /* 计算到目前为止真正流逝的时间 */
    TIMEVAL real_total_sleep_time = total_sleep_time + overrun;

    s_timer_entry *row;
    /* 遍历定时事件入口 */
    for(i=0, row = timers; i <= last_timer_raw; i++, row++)
    {
        if (row->state & TIMER_ARMED) /* if row is active */
        {
            if (row->val <= real_total_sleep_time) /* to be trigged 如果已经超时，则需要触发*/
            {
                if (!row->interval) /* if simply outdated 如果该事件为单次触发，则将该事件状态设置为已触发*/
                {
                    row->state = TIMER_TRIG; /* ask for trig */
                }
                else /* or period have expired 如果该事件为周期触发*/
                {
                    /* set val as interval, with 32 bit overrun correction, */
                    /* modulo for 64 bit not available on all platforms     */
                    /* 校正定时时间 */
                    row->val = row->interval - (overrun % (UNS32)row->interval);
                    /* ask for trig, periodic */
                    /* 状态置为已周期触发 */
                    row->state = TIMER_TRIG_PERIOD; 
                    /* Check if this new timer value is the soonest */
                    /* 更新定时器唤醒时间 */
                    if(row->val < next_wakeup)
                        next_wakeup = row->val;
                }
            }
            else/* 没有超时，不需要触发 */
            {
                /* Each armed timer value in decremented. */
                /* 更新该事件唤醒时间 */
                row->val -= real_total_sleep_time;

                /* Check if this new timer value is the soonest */
                /* 更新定时器唤醒时间 */
                if(row->val < next_wakeup)
                    next_wakeup = row->val;
            }
        }
    }

    /* Remember how much time we should sleep. */
    total_sleep_time = next_wakeup;

    /* Set timer to soonest occurence */
    setTimer(next_wakeup);

    /* Then trig them or not. */
    /* 遍历所有定时事件接口 */
    for(i=0, row = timers; i<=last_timer_raw; i++, row++)
    {
        /* 如果该事件已经被触发(单次触发/周期触发) */
        if (row->state & TIMER_TRIG)
        {
            /* 将单次触发更新为空闲，周期触发的更新为被占用 */
            row->state &= ~TIMER_TRIG; /* reset trig state (will be free if not periodic) */
            /* 需要触发的事件，调用一下回调函数 */
            if(row->callback)
                (*row->callback)(row->d, row->id); /* trig ! */
        }
    }
}
