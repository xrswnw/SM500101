#ifndef _ANYID_SM5001_SYSTICK_HL_H
#define _ANYID_SM5001_SYSTICK_HL_H


#include "AnyID_SM5001_Config.h"


extern vu32 g_nSysTick;
extern vs32 g_nSysDelay;

#define STICK_1MS                           120000                    //ϵͳʱ��



void STick_InitSysTick(void);

#define STick_StartSysTick()                SysTick->CTRL  |= SysTick_CTRL_ENABLE_Msk
#define STick_StopSysTick()                 SysTick->VAL = 0;  SysTick->CTRL  &= (~SysTick_CTRL_ENABLE_Msk)

#define STick_SetDelayTime(delay)           (g_nSysDelay = delay) //��һ��TickΪ��λ
#define STick_IsDelayTimeOver()             (g_nSysDelay <= 0)



#define STICK_TIME_MS                       5

#define STICK_1S                            (1000 / STICK_TIME_MS)

#define STICK_1M                            (STICK_1S * 60)
#define STICK_1H                            (STICK_1M * 60)

#endif

