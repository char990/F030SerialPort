/*
 * MyPrintf.h
 *
 *  Created on: Sep 12, 2023
 *      Author: lq
 */

#ifndef INC_MYPRINTF_H_
#define INC_MYPRINTF_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "stdio.h"

int MyPrintf(const char *fmt, ...);

int MyPutchar(const char c);
int MyPuts(const char *s);


void PrintUint8(const uint8_t *buf, int len, const char *abcd);

void PrintUint16(const uint16_t *buf, int len, const char *abcd);

void PrintUint32(const uint32_t *buf, int len, const char *abcd);

#ifdef __cplusplus
}
#endif

#endif /* INC_MYPRINTF_H_ */
