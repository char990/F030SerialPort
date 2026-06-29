/*
 * Tasks.c
 *
 *  Created on: Sep 12, 2023
 *      Author: lq
 */
#include "main.h"
#include "Tasks.h"
#include "TaskHb.h"
#include "TaskSp.h"

#include "MyPrintf.h"

uint8_t wdt;

uint8_t conspicuity_changed;
uint8_t st_conspicuity;
uint8_t pwm_changed;
uint8_t st_pwm;
uint8_t st_bootup;

uint16_t st_lux[2];
uint16_t st_flasherCurrent[2];

void TasksRun()
{
	TaskHbInit();
	TaskSpInit();

	MyPrintf("\nBuild = %s %s\n", __DATE__, __TIME__);

	while (1)
	{
		TaskHb();
		TaskSpRx();
	};
}
