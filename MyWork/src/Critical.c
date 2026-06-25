/*
 * Critical.c
 *
 *  Created on: Dec 1, 2023
 *      Author: lq
 */
#include "Critical.h"

uint32_t __Get_PRIMASK_n_Disable_irq()
{
	uint32_t priMask_flag = __get_PRIMASK();
	__disable_irq();
	return priMask_flag;
}

