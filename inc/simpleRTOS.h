/*
 * simpleRTOS.h
 *
 *  Created on: Aug 14, 2025
 *      Author: brachiGH
 */

#ifndef SIMPLERTOS_H_
#define SIMPLERTOS_H_

#include "simpleRTOSDefinitions.h"
#define __ICSR (*((volatile uint32_t *)0xE000ED04))

/**
  \brief   Disable IRQ Interrupts
  \details Disables IRQ interrupts by setting the I-bit in the CPSR.
  Can only be executed in Privileged modes.
 */
__STATIC_FORCEINLINE void __sDisable_irq(void)
{
  __asm volatile("cpsid i" : : : "memory");
}

/**
  \brief   Enable IRQ Interrupts
  \details Enables IRQ interrupts by clearing the I-bit in the CPSR.
           Can only be executed in Privileged modes.
 */
__STATIC_FORCEINLINE void __sEnable_irq(void)
{
  __asm volatile("cpsie i" : : : "memory");
}

extern void SysTick_Handler(void);
void SVC_Handler(void);

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
void sRTOSTaskUpdatePriority(sTaskHandle_t *taskHandle, sPriority_t priority);
void sRTOSTaskStop(sTaskHandle_t *taskHandle);
void sRTOSTaskResume(sTaskHandle_t *taskHandle);
void sRTOSTaskDelete(sTaskHandle_t *taskHandle);
void sRTOSTaskDelay(sUBaseType_t duration_ms);

__attribute__((always_inline)) static __inline void sRTOSTaskYield(void)
{
  // __ICSR = (1u << 26); // changes SysTick exception state to pending
  __asm volatile("svc #0");
}

sRTOS_StatusTypeDef sRTOSTimerCreate(
    sTimerFunc_t timerTask,
    sUBaseType_t id,
    sUBaseType_t period,
    sUBaseType_t autoReload,
    sTimerHandle_t *timerHandle);
void sRTOSTimerStop(sTimerHandle_t *timerHandle);
void sRTOSTimerResume(sTimerHandle_t *timerHandle);
void sRTOSTimerDelete(sTimerHandle_t *timerHandle);
void sRTOSTimerUpdatePeriode(sTimerHandle_t *timerHandle, sUBaseType_t period);

void sRTOSSemaphoreCreate(sSemaphore_t *sem, sBaseType_t n);
void sRTOSSemaphoreGive();
void sRTOSSemaphoreTake();

void sRTOSMutexCreate(sSemaphore_t *sem);
void sRTOSMutexGive();
void sRTOSMutexTake();

#endif /* SIMPLERTOS_H_ */
