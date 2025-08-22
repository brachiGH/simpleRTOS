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

void SysTick_Handler(void);
void SVC_Handler(void);

/**
 * @brief   Enter a critical section (disable IRQ interrupts).
 * @details Executes CPSID I. Only valid in privileged mode.
 *          Pairs with __sCriticalRegionEnd().
 * @note    A critical section must be kept as short as possible.
 */
__STATIC_FORCEINLINE__ void __sCriticalRegionBegin(void)
{
  __asm volatile("cpsid i" : : : "memory");
}

/**
 * @brief   Exit a critical section (enable IRQ interrupts).
 * @details Executes CPSIE I. Only valid in privileged mode.
 */
__STATIC_FORCEINLINE__ void __sCriticalRegionEnd(void)
{
  __asm volatile("cpsie i" : : : "memory");
}

/**
 * @brief Initialize core RTOS infrastructure.
 *
 * Configures SysTick to generate interrupts every __sRTOS_SENSIBILITY
 * and creates the idle task.
 *
 * @param BUS_FREQ Core/system clock frequency in Hz.
 *
 * @retval sRTOS_OK Initialization succeeded.
 * @retval sRTOS_ERROR Initialization failed.
 *
 * @note Must be called before creating tasks that depend on the scheduler tick.
 * @warning Call only once before starting the scheduler.
 */
sRTOS_StatusTypeDef sRTOSInit(sUBaseType_t BUS_FREQ);

/**
 * @brief Start the scheduler.
 *
 * Switches to running the highest-priority ready task, then enables
 * periodic tick interrupts. Does not returns only if the scheduler stops.
 *
 * @note Further tasks/timers may still be created after this call if the
 *       implementation supports dynamic creation.
 */
extern void sRTOSStartScheduler(void);

/**
 * @brief Create a task.
 *
 * Allocates/initializes a TCB and stack, assigns priority, and inserts
 * the task into the ready list.
 *
 * @param task           Entry function (should never returns. If it does it returns to an infinit loop).
 * @param name           Descriptive name (may be used for debug; can be NULL).
 * @param arg            Argument passed to task function.
 * @param stacksizeWords Stack depth in 32-bit words (not bytes).
 * @param priority       Task priority (higher value => higher priority).
 * @param taskHandle     Output: handle to the created task (must not be NULL).
 * @param fpsMode        Implementation-defined flag for floating point context.
 *
 * @retval sRTOS_OK Task created.
 * @retval sRTOS_ERROR Allocation or parameter failure.
 *
 * @note Can be called before or after the scheduler starts.
 * @warning Provide sufficient stack; stack overflow behavior is undefined.
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
 * @brief Change an existing task's priority.
 *
 * @param taskHandle Task to modify.
 * @param priority   New priority.
 *
 * @note Will not cause an immediate context switch if the running task
 *       priority is lowered or another becomes highest.
 */
void sRTOSTaskUpdatePriority(sTaskHandle_t *taskHandle, sPriority_t priority);

/**
 * @brief Stop (suspend) a task.
 *
 * @param taskHandle Task to suspend; if NULL uses current task.
 *
 * @note Stopping the running task triggers a yield.
 * @warning Undefined behavior if the handle is invalid.
 */
void sRTOSTaskStop(sTaskHandle_t *taskHandle);

/**
 * @brief Resume (unsuspend) a task.
 *
 * @param taskHandle Task to resume.
 *
 * @note Causes a yield if the resumed task has higher priority than current.
 */
void sRTOSTaskResume(sTaskHandle_t *taskHandle);

/**
 * @brief Delete a task and free its resources.
 *
 * @param taskHandle Task to delete; if NULL deletes current task.
 *
 * @note Deleting the running task triggers a yield.
 * @warning Undefined behavior if the handle is invalid. Do not use the handle after deletion.
 */
void sRTOSTaskDelete(sTaskHandle_t *taskHandle);

/**
 * @brief Delay (sleep) the calling task.
 *
 * @param duration_ms Time in milliseconds to delay (converted to ticks).
 *
 * @note Only valid from task context.
 */
void sRTOSTaskDelay(sUBaseType_t duration_ms);

/**
 * @brief Voluntarily yield the processor.
 *
 * Forces a SVC to request a scheduling decision.
 *
 * @note Use to allow equal-priority tasks to share CPU cooperatively.
 */
__STATIC_FORCEINLINE__ void sRTOSTaskYield(void)
{
  __asm volatile("svc #0");
}

/**
 * @brief Create a software timer.
 *
 * @param timerTask   Callback executed when the timer expires.
 * @param id          User-defined identifier passed.
 * @param period      Period in ticks (or relative time units).
 * @param autoReload  Non-zero for periodic; zero for one-shot.
 * @param timerHandle Timer handle.
 *
 * @retval sRTOS_OK Timer created.
 * @retval sRTOS_ERROR Failure (allocation/parameters).
 *
 * @note Can be created before or after scheduler start.
 */
sRTOS_StatusTypeDef sRTOSTimerCreate(
    sTimerFunc_t timerTask,
    sUBaseType_t id,
    sBaseType_t period,
    sUBaseType_t autoReload,
    sTimerHandle_t *timerHandle);

/**
 * @brief Stop a timer.
 *
 * @param timerHandle Timer to stop.
 *
 * @note If a timer is currently executing its callback, this request
 *       affects subsequent cycles (callback continues until it returns).
 * @warning Undefined behavior if the handle is invalid.
 */
void sRTOSTimerStop(sTimerHandle_t *timerHandle);

/**
 * @brief Start or resume a timer.
 *
 * @param timerHandle Timer to resume.
 *
 * @note Resets, next expiry relative to now.
 *
 * @warning Undefined behavior if the handle is invalid.
 */
void sRTOSTimerResume(sTimerHandle_t *timerHandle);

/**
 * @brief Delete a timer and free its resources.
 *
 * @param timerHandle Timer to delete.
 *
 * @warning Do NOT delete a timer while its callback is executing; doing
 *          so may cause use-after-free.
 * @warning Undefined behavior if the handle is invalid.
 */
void sRTOSTimerDelete(sTimerHandle_t *timerHandle);

/**
 * @brief Update a timer's period.
 *
 * @param timerHandle Timer to modify.
 * @param period      New period in ticks.
 *
 * @note Takes effect for the next cycle.
 * @warning Undefined behavior if the handle is invalid.
 */
void sRTOSTimerUpdatePeriod(sTimerHandle_t *timerHandle, sBaseType_t period);

/**
 * @brief Convert milliseconds to RTOS ticks.
 *
 * @param timeoutMS Milliseconds.
 * @return Ticks corresponding to timeoutMS.
 *
 * @note Rounds toward zero; ensure resolution suits timing requirements.
 */
__STATIC_FORCEINLINE__ sUBaseType_t srMS_TO_TICKS(sUBaseType_t timeoutMS)
{
  return timeoutMS * (__sRTOS_SENSIBILITY / 1000);
}

/**
 * @brief Create (initialize) a counting semaphore.
 *
 * @param sem Pointer to semaphore object.
 * @param n   Initial count (no maximum).
 *
 * @note Must be called before give/take operations.
 */
void sRTOSSemaphoreCreate(sSemaphore_t *sem, sBaseType_t n);

/**
 * @brief Give (increment) a semaphore.
 *
 * @param sem Semaphore to give.
 *
 * @warning Does not yield automatically while waiting, unless quantum finishs.
 */
void sRTOSSemaphoreGive(sSemaphore_t *sem);

/**
 * @brief Take (decrement) a semaphore with timeout (non-cooperative wait).
 *
 * @param sem          Semaphore to take.
 * @param timeoutTicks Max ticks to wait (0 = poll).
 *
 * @retval true Obtained before timeout.
 * @retval false Timeout occurred.
 *
 * @note Does not yield while waiting; uses busy/periodic check strategy.
 */
sbool_t sRTOSSemaphoreTake(sSemaphore_t *sem, sUBaseType_t timeoutTicks);

/**
 * @brief Take a semaphore with cooperative waiting (yields while waiting).
 *
 * @param sem          Semaphore to take.
 * @param timeoutTicks Max ticks to wait (0 = poll).
 *
 * @retval true Obtained before timeout.
 * @retval false Timeout occurred.
 *
 * @note Suitable for longer waits; allows other tasks to run immedialty.
 */
sbool_t sRTOSSemaphoreCooperativeTake(sSemaphore_t *sem, sUBaseType_t timeoutTicks);

/**
 * @brief Create (initialize) a mutex.
 *
 * @param mux Pointer to mutex object.
 *
 * @note Mutex provides ownership semantics (priority inheritance unspecified).
 */
void sRTOSMutexCreate(sMutex_t *mux);

/**
 * @brief Release (give) a mutex the calling task owns.
 *
 * @param mux Mutex to release.
 *
 * @retval true Released and possibly unblocked a waiting task.
 * @retval false Calling task was not the owner or invalid handle.
 *
 * @note May cause a yield if a higher-priority task is blocked waiting for the mutex to release.
 */
sbool_t sRTOSMutexGive(sMutex_t *mux);

/**
 * @brief Release a mutex from ISR (no ownership check).
 *
 * @param mux Mutex to release.
 *
 * @retval true Released.
 * @retval false Invalid handle or not releasable.
 *
 * @warning Ownership is not validated; use only where safe.
 * @note Does not yield; but the task waiting to the release of the mutex inherites MaxPriority
 * and will almost always run immediatly after the isr.
 */
sbool_t sRTOSMutexGiveFromISR(sMutex_t *mux);

/**
 * @brief Acquire (take) a mutex.
 *
 * @param mux          Mutex to take.
 * @param timeoutTicks Max ticks to wait (0 = poll).
 *
 * @retval true Acquired; caller becomes owner.
 * @retval false Timeout or failure.
 *
 * @note May yield while waiting.
 * @warning Deadlock possible if not used with care.
 */
sbool_t sRTOSMutexTake(sMutex_t *mux, sUBaseType_t timeoutTicks);

/**
 * @brief Waits for a notification sent to the calling task.
 *
 * Blocks the calling task until a notification is received or until the timeout
 * period expires. When a notification is received, the associated message value
 * is returned to the caller.
 *
 * @return If no notification is received before the timeout expires, returns 0.
 *
 * @note Intended to be called from task context.
 * @warning Not safe to call from an interrupt context.
 */
sUBaseType_t sRTOSTaskNotifyTake(sUBaseType_t timeoutTicks);

/**
 * @brief Sends a notification with a message to a target task.
 *
 * Posts a notification to specified task, optionally waking it if it is
 * blocked waiting in sRTOSTaskNotifyTake(). The provided message value is
 * delivered to the receiving task.
 *
 * @param taskToNotify Pointer to the handle of the task to notify. Must not be NULL.
 * @param message uint32 value to deliver with the notification.
 *
 * @note Intended to be called from task context.
 * @warning Not safe to call from an interrupt context.
 */
void sRTOSTaskNotify(sTaskHandle_t *taskToNotify, sUBaseType_t message);

/**
 * @brief Sends a notification with a message to a target task.
 *
 * Posts a notification to the specified task, optionally waking it if it is
 * blocked waiting in sRTOSTaskNotifyTake(). The provided message value is
 * delivered to the receiving task.
 *
 * @param taskToNotify Pointer to the handle of the task to notify. Must not be NULL.
 * @param message uint32 value to deliver with the notification.
 *
 * @note Intended to be called from ISR context.
 */
void sRTOSTaskNotifyFromISR(sTaskHandle_t *taskToNotify, sUBaseType_t message);

/**
 * @brief Creates/initializes a queue object.
 *
 * Initializes the queue referenced by queueHandle with the specified capacity
 * and item size. The queue must be created before it can be used with send or
 * receive operations.
 *
 * @param queueHandle Pointer to the queue handle to initialize. Must not be NULL.$
 * @param queueLengh Number of items the queue can hold (capacity).
 * @param itemSize Size, in bytes, of each item stored in the queue.
 *
 * @note This function must be called before sRTOSQueueSend() or sRTOSQueueReceive().
 */
void sRTOSQueueCreate(sQueueHandle_t *queueHandle, sUBaseType_t queueLengh, sUBaseType_t itemSize);

/**
 * @brief Receives (dequeues) an item from a queue.
 *
 * If the queue is empty, this function blocks for up to timeoutTicks waiting for
 * an item to become available.
 *
 * @param queueHandle Pointer to the queue handle. Must reference a created queue.
 * @param itemPtr Pointer to a buffer large enough to hold one queue item; on success,
 *                the received item is copied into this buffer.
 * @param timeoutTicks Number of RTOS ticks to wait.
 *
 * @retval true An item was successfully received and copied to itemPtr.
 * @retval false No item was received before timeout.
 *
 * @note Intended to be called from task context.
 * @warning Not safe to call from an interrupt context.
 */
sbool_t sRTOSQueueReceive(sQueueHandle_t *queueHandle, void *itemPtr, sUBaseType_t timeoutTicks);

/**
 * @brief Sends (enqueues) an item to a queue.
 *
 * If the queue is full, this function blocks for up to timeoutTicks waiting for
 * space to become available.
 *
 * @param queueHandle Pointer to the queue handle. Must reference a created queue.
 * @param itemPtr Pointer to the item data to send; the data at this address is copied
 * @param timeoutTicks Number of RTOS ticks to wait.
 *
 * @retval true The item was successfully enqueued.
 * @retval false The item could not be enqueued before timeout, or memory allocation failed.
 *
 * @note Intended to be called from task context.
 * @warning Not safe to call from an interrupt context.
 */
sbool_t sRTOSQueueSend(sQueueHandle_t *queueHandle, void *itemPtr, sUBaseType_t timeoutTicks);

/**
 * @brief Sends (enqueues) an item to a queue.
 *
 * If the queue is full, this function blocks for up to timeoutTicks waiting for
 * space to become available.
 *
 * @param queueHandle Pointer to the queue handle. Must reference a created queue.
 * @param itemPtr Pointer to the item data to send; the data at this address is copied
 * @param timeoutTicks Number of RTOS ticks to wait.
 *
 * @retval true The item was successfully enqueued.
 * @retval false if the queue is full or memory allocation failed.
 *
 * @note Intended to be called from ISR context.
 */
sbool_t sRTOSQueueSendFromISR(sQueueHandle_t *queueHandle, void *itemPtr);

#endif /* SIMPLERTOS_H_ */
