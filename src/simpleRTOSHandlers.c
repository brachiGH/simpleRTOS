/*
 * simpleRTOSHandlers.c
 *
 *  Created on: Aug 15, 2025
 *      Author: benhih
 */

#include "simpleRTOS.h"

volatile sUBaseType_t _sTickCount = 0;
volatile sUBaseType_t _sIsTimerRunning = 0;

extern void sScheduler_Handler(void);
extern void sTimerReturn_Handler(void);

__attribute__((weak)) void SysTick_Handler(void) {}

__attribute__((weak)) void SVC_Handler(void) {}
