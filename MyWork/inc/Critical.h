/*
 * Critical.h
 *
 *  Created on: Aug 22, 2023
 *      Author: lq
 */

#ifndef INC_UTILS_CRITICAL_H_
#define INC_UTILS_CRITICAL_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "stdint.h"
#include "cmsis_gcc.h"

#define _CATAB(a, b) a##b
#define CATAB(a, b) _CATAB(a, b)

	extern uint32_t __Get_PRIMASK_n_Disable_irq();

#define ATOMIC_CODE() \
	for (uint32_t CATAB(_priMask_, __LINE__) = __Get_PRIMASK_n_Disable_irq(), CATAB(_cnt_flag_, __LINE__) = 0; CATAB(_cnt_flag_, __LINE__)++ == 0; __set_PRIMASK(CATAB(_priMask_, __LINE__)))

#ifdef __cplusplus
}
#endif

#endif /* INC_UTILS_CRITICAL_H_ */
