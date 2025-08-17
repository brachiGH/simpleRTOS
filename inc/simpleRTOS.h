/*
 * simpleRTOS.h
 *
 *  Created on: Aug 14, 2025
 *      Author: brachiGH
 */

#ifndef SIMPLERTOS_H_
#define SIMPLERTOS_H_

#include "simpleRTOSDefinitions.h"
#include "simpleRTOSConfig.h"

#define __ICSR (*((volatile uint32_t *)0xE000ED04))

/**
  @brief   Disable IRQ Interrupts
  @attention Disables IRQ interrupts by setting the I-bit in the CPSR.
             Can only be executed in Privileged modes.
 */
__STATIC_FORCEINLINE__ void __sCriticalRegionBegin(void)
{
  __asm volatile("cpsid i" : : : "memory");
}

/**
  @brief   Enable IRQ Interrupts
  @attention Enables IRQ interrupts by clearing the I-bit in the CPSR.
             Can only be executed in Privileged modes.
 */
__STATIC_FORCEINLINE__ void __sCriticalRegionEnd(void)
{
  __asm volatile("cpsie i" : : : "memory");
}

extern void SysTick_Handler(void);
void SVC_Handler(void);


/**
 * @brief Initializes the RTOS.
 * @param BUS_FREQ The system core clock frequency (used to configure the SysTick timer).
 * @attention sRTOSInit sets up the SysTick timer to generate interrupts every 
            __sRTOS_SENSIBILITY ticks, and creates an idle task that runs 
            when no other task is ready to execute.
 */
sRTOS_StatusTypeDef sRTOSInit(sUBaseType_t BUS_FREQ);

/**
 * @brief Starts the scheduler by running the highest-priority task created 
          and enabling the SysTick timer.
 */
extern void sRTOSStartScheduler(void);

/**
 * @brief Creates a new task.
 * @attention Tasks can still be created after sRTOSStartScheduler() is called.
              sRTOSTaskCreate updates the `nextTask` field in the `taskHandle`.
 */
sRTOS_StatusTypeDef sRTOSTaskCreate(
    sTaskFunc_t task,
    char *name,
    void *arg,
    sUBaseType_t stacksizeWords,
    sPriority_t priority,
    sTaskHandle_t *taskHandle,
    sUBaseType_t fpsMode);

/**
 * @brief Updates a task's priority.
 * @attention This function can be heavy if you have many tasks.
              When executed, the function enters a critical region.
 */
void sRTOSTaskUpdatePriority(sTaskHandle_t *taskHandle, sPriority_t priority);

/**
 * @brief Stop Task
 * @note Stoping the currently running task will yeild.
 */
void sRTOSTaskStop(sTaskHandle_t *taskHandle);

/**
 * @brief Resume Task
 * @note Will yeild if the currently running task has a lower priority then the resumed task.
 */
void sRTOSTaskResume(sTaskHandle_t *taskHandle);

/**
 * @brief Delete Task
 * @attention If provided a none existing taskHandle is provided nothing happens.
              26 bytes are left from every deleted task.
 * @note Deleting the currently running task will yeild.
 */
void sRTOSTaskDelete(sTaskHandle_t *taskHandle);

/**
 * @note only works on tasks.
 */
void sRTOSTaskDelay(sUBaseType_t duration_ms);

/**
 * @note runs the scheduler. 
 */
__STATIC_FORCEINLINE__ void sRTOSTaskYield(void)
{
  // __ICSR = (1u << 26); // changes SysTick exception state to pending
  __asm volatile("svc #0");
}

/**
 * @brief Creates a new timer.
 * @attention timer can still be created after sRTOSStartScheduler() is called.
 */
sRTOS_StatusTypeDef sRTOSTimerCreate(
    sTimerFunc_t timerTask,
    sUBaseType_t id,
    sBaseType_t period,
    sUBaseType_t autoReload,
    sTimerHandle_t *timerHandle);

/**
 * @brief Stop Timer
 * @attention will not stop the timer if it is running. 
 */
void sRTOSTimerStop(sTimerHandle_t *timerHandle);

/**
 * @brief Resume Timer
 */
void sRTOSTimerResume(sTimerHandle_t *timerHandle);

/**
 * @brief Delete Timer
 * @note If a NULL or invalid timerHandle is provided, no action is taken.
 * @attention If a timer deletes itself or is deleted from an ISR,
              the timer continues executing until it returns.
              Deleting while the timer is still running causes use-after-free,
              Do NOT delete a timer until the timer is no longer in use (Because it memory is freed).
 */
void sRTOSTimerDelete(sTimerHandle_t *timerHandle);

/**
 * @brief Update Timer Periode.
 */
void sRTOSTimerUpdatePeriode(sTimerHandle_t *timerHandle, sBaseType_t period);

/**
 * @brief MS to ticks
 */
__STATIC_FORCEINLINE__ sUBaseType_t srMS_TO_TICKS(sUBaseType_t timeoutMS)
{
  return timeoutMS * (__sRTOS_SENSIBILITY / 1000);
}

/**
 * @brief Create Semaphore.
 */
void sRTOSSemaphoreCreate(sSemaphore_t *sem, sBaseType_t n);

/**
 * @brief Gives a semaphore.
 * @attention This call will not yield.
 */
void sRTOSSemaphoreGive(sSemaphore_t *sem);

/**
 * @brief Takes a semaphore.
 * @attention This call will not yield while waiting.
 */
sbool_t sRTOSSemaphoreTake(sSemaphore_t *sem, sUBaseType_t timeoutTicks);


/**
 * @brief Takes a semaphore.
 * @attention This call will yield while waiting. 
 *            (Use when the timeout is longer than one time quantum.)
 */

sbool_t sRTOSSemaphoreCooperativeTake(sSemaphore_t *sem, sUBaseType_t timeoutTicks);

/**
 * @brief Create Mutex.
 */
void sRTOSMutexCreate(sMutex_t *mux);

/**
 * @brief Releases a mutex (only the owning task can release it).
 * @attention This call will yield.
 */
sbool_t sRTOSMutexGive(sMutex_t *mux);

/**
 * @brief Releases a mutex without checking ownership.
 * @attention This call will not yield.
 */
sbool_t sRTOSMutexGiveFromISR(sMutex_t *mux);

/**
 * @brief Takes a mutex.
 * @note If a task successfully takes the mutex, it becomes the owner.
 * @attention This call will yield while waiting.
 */
sbool_t sRTOSMutexTake(sMutex_t *mux, sUBaseType_t timeoutTicks);

#endif /* SIMPLERTOS_H_ */
