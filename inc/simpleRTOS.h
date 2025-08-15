/*
 * simpleRTOS.h
 *
 *  Created on: Aug 14, 2025
 *      Author: brachiGH
 */

#ifndef SIMPLERTOS_H_
#define SIMPLERTOS_H_

#include "simpleRTOSDefinitions.h"

extern void SysTick_Handler(void);

sRTOS_StatusTypeDef sRTOSInit(sUBaseType_t BUS_FREQ);
extern void sRTOSStartScheduler(void);

sRTOS_StatusTypeDef sRTOSTaskCreate(
    sTaskFunc_t task,
    char *name,
    void *arg,
    sUBaseType_t stacksizeWords,
    sPriority_t priority,
    sTaskHandle_t *taskHandle,
    sUBaseType_t fpsMode);
void sRTOSTaskYield();
void sRTOSTaskStop(sTaskHandle_t *taskHandle);
void sRTOSTaskStart(sTaskHandle_t *taskHandle);
void sRTOSTaskDelete(sTaskHandle_t *taskHandle);

sRTOS_StatusTypeDef sRTOSTimerCreate(
    sTimerFunc_t timer,
    sUBaseType_t id,
    sUBaseType_t period,
    sUBaseType_t autoReload,
    sTimerHandle_t *taskHandle,
    sUBaseType_t fpsMode);
void sRTOSTimerDelete(sTimerHandle_t *timerHandle);
void sRTOSTimerUpdatePeriode(sTimerHandle_t *timerHandle);

void sRTOSSemaphoreCreate(sSemaphore_t *sem, sBaseType_t n);
void sRTOSSemaphoreGive();
void sRTOSSemaphoreTake();

void sRTOSMutexCreate(sSemaphore_t *sem);
void sRTOSMutexGive();
void sRTOSMutexTake();

void sRTOSDelay();

#endif /* SIMPLERTOS_H_ */
