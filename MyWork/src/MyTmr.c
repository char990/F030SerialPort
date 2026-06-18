/*
 * MyTmr.c
 *
 *  Created on: Sep 12, 2023
 *      Author: lq
 */
#include "MyTmr.h"

void MakeMsTmrExpired(msTmr_t *tmr)
{
	tmr->tickstart = HAL_GetTick() - 1;
	tmr->wait = 1;
}

void SetMsTmr(msTmr_t *tmr, ms_t delay)
{
	tmr->tickstart = HAL_GetTick();
	tmr->wait = delay;
}

void AddMsTmr(msTmr_t *tmr, ms_t delay)
{
	tmr->tickstart = (tmr->wait != 0) ? (tmr->tickstart + tmr->wait) : HAL_GetTick();
	tmr->wait = delay;
}

void ClrMsTmr(msTmr_t *tmr)
{
	SetMsTmr(tmr, 0);
}

int IsMsTmrExpired(msTmr_t *tmr)
{
	return (tmr->wait != 0) && ((ms_t)(HAL_GetTick() - tmr->tickstart) >= tmr->wait);
}

int IsMsTmrSet(msTmr_t *tmr)
{
	return tmr->wait;
}

void SetUsTmr(usTmr_t *tmr, us_t delay)
{
	tmr->tickstart = htim14.Instance->CNT;
	tmr->wait = (delay >= US_MAX) ? 65535 : delay * US_TIM_MHZ;
}

void ClrUsTmr(usTmr_t *tmr)
{
	SetUsTmr(tmr, 0);
}

int IsUsTmrExpired(usTmr_t *tmr)
{
	return (tmr->wait != 0) && ((us_t)((us_t)(htim14.Instance->CNT) - tmr->tickstart) >= tmr->wait);
}

int IsUsTmrSet(usTmr_t *tmr)
{
	return tmr->wait;
}

void Delay_us(us_t delay)
{
	usTmr_t tmr;
	SetUsTmr(&tmr, delay);
	while ((us_t)((us_t)(htim14.Instance->CNT) - tmr.tickstart) < tmr.wait)
		;
}

static __IO uint32_t tick_ms;
static __IO uint32_t tick_seconds;

extern __IO uint32_t uwTick;
extern HAL_TickFreqTypeDef uwTickFreq;
void HAL_IncTick()
{
	uwTick += uwTickFreq;
	tick_ms += uwTickFreq;
	if (tick_ms >= 1000)
	{
		tick_ms -= 1000;
		tick_seconds++;
	}
}

uint32_t Timestamp()
{
	return tick_seconds;
}
