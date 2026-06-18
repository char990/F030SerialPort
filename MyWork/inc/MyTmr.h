/*
 * MyTmr.h
 *
 *  Created on: Sep 12, 2023
 *      Author: lq
 */

#ifndef INC_MYTMR_H_
#define INC_MYTMR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "tim.h"

extern volatile uint32_t bitCnt;
uint32_t GetBitCnt();

uint32_t Timestamp();

// msTmr_t uses HAL tick
typedef uint32_t ms_t;
typedef struct msTmr_t
{
	  ms_t tickstart;
	  ms_t wait;
}msTmr_t;

void MakeMsTmrExpired(msTmr_t * tmr);

void SetMsTmr(msTmr_t * tmr, ms_t delay);
void AddMsTmr(msTmr_t *tmr, ms_t delay);

void ClrMsTmr(msTmr_t * tmr);

int IsMsTmrExpired(msTmr_t * tmr);

int IsMsTmrSet(msTmr_t * tmr);

// usTmr_t uses 16-bit timer
typedef uint16_t us_t;
#define US_TIM_MHZ 48		// 47.9232M
#define US_MAX ((us_t)-1/US_TIM_MHZ)	// 65535/48=1365
typedef struct usTmr_t
{
	  us_t tickstart;
	  us_t wait;
}usTmr_t;

void SetUsTmr(usTmr_t * tmr, us_t delay);

void ClrUsTmr(usTmr_t * tmr);

int IsUsTmrExpired(usTmr_t * tmr);

int IsUsTmrSet(usTmr_t * tmr);

void Delay_us(us_t delay);

#ifdef __cplusplus
}
#endif

#endif /* INC_MYTMR_H_ */
