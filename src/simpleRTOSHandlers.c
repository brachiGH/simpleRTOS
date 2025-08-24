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

sUBaseType_t sGetTick()
{
  __sCriticalRegionBegin();
  sUBaseType_t temp = _sTickCount;
  __sCriticalRegionEnd();
  return temp;
}

__attribute__((weak)) void SysTick_Handler(void) {}

__attribute__((weak)) void SVC_Handler(void) {}
