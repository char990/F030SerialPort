/*
 * TaskHb.c
 *
 *  Created on: Sep 12, 2023
 *      Author: lq
 */
#include "TaskHb.h"
#include "MyTmr.h"
#include "main.h"
#include "Tasks.h"
#include "iwdg.h"

myPt_t ptHb;
#define this_pt (&ptHb)

msTmr_t tmrHb;
#define this_tmr (&tmrHb)

void TaskHbInit()
{
	PT_Reset(this_pt);
}

uint8_t TaskHb()
{
	PT_BEGIN(this_pt);
	for (;;)
	{
		SetMsTmr(this_tmr, 500);
		PT_WAIT_UNTIL(this_pt, IsMsTmrExpired(this_tmr));
		HAL_GPIO_TogglePin(HB_GPIO_Port, HB_Pin);
		if (wdt == TASK_ALL)
		{
			wdt = 0;
#ifndef DEBUG
			HAL_IWDG_Refresh(&hiwdg);
#endif
		}
	}
	PT_END(this_pt);
}
