# simpleRTOS

This is a simple RTOS designed for Cortex-M4 microcontrollers, current features are timers, semaphores, mutexes, queues, and notifications.

Primarily for learning and experimentation. It can a preemptive or a non-preemptive, priority-based scheduler that uses `SysTick`, and is designed with minimal memory footprint. 

Note: This is not intended for production use. And only compiles with gcc.

## Achitecture

- O(1) scheduler uses a bitmap for selecting the highest-priority runnable task.
- 32 priority levels mapped to bits in a bitmap; tasks at the same level are kept in a circular doubly linked list for O(1) enqueue/dequeue and fair round-robin within that priority level.
- Tasks for mutexes or notifications inherit priority to mitigate priority inversion.

### Scheduler 

![Scheduler overview](./scheduler-diagram.png)

## Configuration

Configure the kernel in `simpleRTOSConfig.h` header:

```c
/* RTOS sensibility is how responsive is the system*/
#define __sRTOS_SENSIBILITY_10MS 100 // above 1MS all function that use MS time as input will works multiples of __sRTOS_SENSIBILITY in this case multipules of 10MS
#define __sRTOS_SENSIBILITY_1MS   1000
#define __sRTOS_SENSIBILITY_500us 2000
#define __sRTOS_SENSIBILITY_250us 4000
#define __sRTOS_SENSIBILITY_100us 10000

#define __sRTOS_SENSIBILITY __sRTOS_SENSIBILITY_500us
/**************************************************/

#define __sUSE_PREEMPTION 1             // if set to 1 the scheduler became preemptive
#define __sQUANTA 2                     // the quanta duration is relative to __sRTOS_SENSIBILITY
                                        // if sensibility is 100us then 1 quanta = 100us
                                        //(note:same priority tasks are rotate)
#define __sTIMER_TASK_STACK_DEPTH 256   // in words
#define __sMAX_DELAY 0xFFFFFFFF
```

# simpleRTOS API Reference

## System Control

### `sRTOSInit`
Initializes the RTOS core infrastructure.
```c
sRTOS_StatusTypeDef sRTOSInit(sUBaseType_t BUS_FREQ);
```
- **@brief:** Configures SysTick and creates the idle task.
- **@param `BUS_FREQ`:** Core/system clock frequency in Hz.
- **@retval `sRTOS_OK`:** Initialization succeeded.
- **@retval `sRTOS_ERROR`:** Initialization failed.
- **@warning:** Must be called only once before starting the scheduler.

### `sRTOSStartScheduler`
Starts the RTOS scheduler.
```c
extern void sRTOSStartScheduler(void);
```
- **@brief:** Begins multitasking. This function does not return unless the scheduler is stopped.
- **@note:** Tasks can still be created after the scheduler has started.

### `__sCriticalRegionBegin`
Enters a critical section.
```c
__STATIC_FORCEINLINE__ void __sCriticalRegionBegin(void);
```
- **@brief:** Disables IRQ interrupts to protect a code section from being preempted.
- **@note:** Critical sections should be kept as short as possible.

### `__sCriticalRegionEnd`
Exits a critical section.
```c
__STATIC_FORCEINLINE__ void __sCriticalRegionEnd(void);
```
- **@brief:** Re-enables IRQ interrupts.

## Task Management

### `sRTOSTaskCreate`
Creates a new task.
```c
sRTOS_StatusTypeDef sRTOSTaskCreate(
  sTaskFunc_t task,
  char *name,
  void *arg,
  sUBaseType_t stacksizeWords,
  sPriority_t priority,
  sTaskHandle_t *taskHandle,
  sUBaseType_t fpsMode);
```
- **@brief:** Allocates and initializes a task control block (TCB) and stack.
- **@param `task`:** The function that implements the task.
- **@param `name`:** A descriptive name for the task (can be NULL).
- **@param `arg`:** Argument passed to the task function.
- **@param `stacksizeWords`:** Stack depth in 32-bit words.
- **@param `priority`:** Task priority (higher value means higher priority).
- **@param `taskHandle`:** Pointer to a handle that will reference the created task.
- **@param `fpsMode`:** Flag for floating-point context saving.
- **@retval `sRTOS_OK`:** Task created successfully.
- **@retval `sRTOS_ERROR`:** Failed to create the task.
- **@warning:** Ensure sufficient stack size is provided to prevent overflow.

### `sRTOSTaskUpdatePriority`
Changes a task's priority.
```c
void sRTOSTaskUpdatePriority(sTaskHandle_t *taskHandle, sPriority_t priority);
```
- **@param `taskHandle`:** The handle of the task to modify.
- **@param `priority`:** The new priority for the task.

### `sRTOSTaskStop`
Suspends a task.
```c
void sRTOSTaskStop(sTaskHandle_t *taskHandle);
```
- **@param `taskHandle`:** The handle of the task to suspend. If NULL, the calling task is suspended.
- **@note:** Suspending the currently running task triggers a context switch.

### `sRTOSTaskResume`
Resumes a suspended task.
```c
void sRTOSTaskResume(sTaskHandle_t *taskHandle);
```
- **@param `taskHandle`:** The handle of the task to resume.
- **@note:** May cause a context switch if the resumed task has a higher priority than the current task.

### `sRTOSTaskDelete`
Deletes a task.
```c
void sRTOSTaskDelete(sTaskHandle_t *taskHandle);
```
- **@param `taskHandle`:** The handle of the task to delete. If NULL, the calling task is deleted.
- **@warning:** Do not use the task handle after the task has been deleted.

### `sRTOSTaskDelay`
Delays the calling task.
```c
void sRTOSTaskDelay(sUBaseType_t duration_ms);
```
- **@param `duration_ms`:** The delay duration in milliseconds.
- **@note:** Can only be called from within a task.

### `sRTOSTaskYield`
Yields the processor.
```c
__STATIC_FORCEINLINE__ void sRTOSTaskYield(void);
```
- **@brief:** Forces a context switch to allow other tasks to run.

## Task Notifications

### `sRTOSTaskNotifyTake`
Waits for a task notification.
```c
sUBaseType_t sRTOSTaskNotifyTake(sUBaseType_t timeoutTicks);
```
- **@brief:** Blocks the calling task until a notification is received or a timeout occurs.
- **@return:** The message value from the notification, or 0 on timeout.
- **@warning:** Not safe to call from an ISR.

### `sRTOSTaskNotify`
Sends a notification to a task.
```c
void sRTOSTaskNotify(sTaskHandle_t *taskToNotify, sUBaseType_t message);
```
- **@param `taskToNotify`:** Handle of the task to notify.
- **@param `message`:** A 32-bit value to send with the notification.
- **@warning:** Not safe to call from an ISR.

### `sRTOSTaskNotifyFromISR`
Sends a notification to a task from an ISR.
```c
void sRTOSTaskNotifyFromISR(sTaskHandle_t *taskToNotify, sUBaseType_t message);
```
- **@param `taskToNotify`:** Handle of the task to notify.
- **@param `message`:** A 32-bit value to send with the notification.

## Timer Management

### `sRTOSTimerCreate`
Creates a software timer.
```c
sRTOS_StatusTypeDef sRTOSTimerCreate(
  sTimerFunc_t timerTask,
  sUBaseType_t id,
  sBaseType_t period,
  sUBaseType_t autoReload,
  sTimerHandle_t *timerHandle);
```
- **@param `timerTask`:** The callback function to execute when the timer expires.
- **@param `id`:** A user-defined ID for the timer.
- **@param `period`:** The timer period in ticks.
- **@param `autoReload`:** If non-zero, the timer is periodic; otherwise, it is a one-shot timer.
- **@param `timerHandle`:** Pointer to a handle that will reference the created timer.
- **@retval `sRTOS_OK`:** Timer created successfully.
- **@retval `sRTOS_ERROR`:** Failed to create the timer.

### `sRTOSTimerStop`
Stops a timer.
```c
void sRTOSTimerStop(sTimerHandle_t *timerHandle);
```
- **@param `timerHandle`:** The handle of the timer to stop.

### `sRTOSTimerResume`
Starts or resumes a timer.
```c
void sRTOSTimerResume(sTimerHandle_t *timerHandle);
```
- **@param `timerHandle`:** The handle of the timer to resume.

### `sRTOSTimerDelete`
Deletes a timer.
```c
void sRTOSTimerDelete(sTimerHandle_t *timerHandle);
```
- **@param `timerHandle`:** The handle of the timer to delete.
- **@warning:** Do not delete a timer from its own callback function to avoid use-after-free issues.

### `sRTOSTimerUpdatePeriod`
Updates a timer's period.
```c
void sRTOSTimerUpdatePeriod(sTimerHandle_t *timerHandle, sBaseType_t period);
```
- **@param `timerHandle`:** The handle of the timer to modify.
- **@param `period`:** The new period in ticks.

## Semaphore Management

### `sRTOSSemaphoreCreate`
Creates a counting semaphore.
```c
void sRTOSSemaphoreCreate(sSemaphore_t *sem, sBaseType_t n);
```
- **@param `sem`:** Pointer to the semaphore object to initialize.
- **@param `n`:** The initial count of the semaphore.

### `sRTOSSemaphoreGive`
Gives (increments) a semaphore.
```c
void sRTOSSemaphoreGive(sSemaphore_t *sem);
```
- **@param `sem`:** The semaphore to give.

### `sRTOSSemaphoreTake`
Takes a semaphore with a non-cooperative wait.
```c
sbool_t sRTOSSemaphoreTake(sSemaphore_t *sem, sUBaseType_t timeoutTicks);
```
- **@param `sem`:** The semaphore to take.
- **@param `timeoutTicks`:** Maximum ticks to wait (busy-wait).
- **@retval `true`:** Semaphore was taken.
- **@retval `false`:** Timeout occurred.

### `sRTOSSemaphoreCooperativeTake`
Takes a semaphore with a cooperative wait.
```c
sbool_t sRTOSSemaphoreCooperativeTake(sSemaphore_t *sem, sUBaseType_t timeoutTicks);
```
- **@param `sem`:** The semaphore to take.
- **@param `timeoutTicks`:** Maximum ticks to wait (yields while waiting).
- **@retval `true`:** Semaphore was taken.
- **@retval `false`:** Timeout occurred.

## Mutex Management

### `sRTOSMutexCreate`
Creates a mutex.
```c
void sRTOSMutexCreate(sMutex_t *mux);
```
- **@param `mux`:** Pointer to the mutex object to initialize.

### `sRTOSMutexGive`
Releases a mutex.
```c
sbool_t sRTOSMutexGive(sMutex_t *mux);
```
- **@param `mux`:** The mutex to release.
- **@retval `true`:** Mutex was released.
- **@retval `false`:** The calling task was not the owner.
- **@note:** May cause a yield if a higher-priority task was waiting.

### `sRTOSMutexGiveFromISR`
Releases a mutex from an ISR.
```c
sbool_t sRTOSMutexGiveFromISR(sMutex_t *mux);
```
- **@param `mux`:** The mutex to release.
- **@warning:** Ownership is not checked.

### `sRTOSMutexTake`
Acquires (takes) a mutex.
```c
sbool_t sRTOSMutexTake(sMutex_t *mux, sUBaseType_t timeoutTicks);
```
- **@param `mux`:** The mutex to take.
- **@param `timeoutTicks`:** Maximum ticks to wait.
- **@retval `true`:** Mutex was acquired.
- **@retval `false`:** Timeout or failure.
- **@warning:** Can lead to deadlock if not used carefully.

## Queue Management

### `sRTOSQueueCreate`
Creates a queue.
```c
void sRTOSQueueCreate(sQueueHandle_t *queueHandle, sUBaseType_t queueLengh, sUBaseType_t itemSize);
```
- **@param `queueHandle`:** Pointer to the queue handle to initialize.
- **@param `queueLengh`:** The maximum number of items the queue can hold.
- **@param `itemSize`:** The size of each item in bytes.

### `sRTOSQueueReceive`
Receives an item from a queue.
```c
sbool_t sRTOSQueueReceive(sQueueHandle_t *queueHandle, void *itemPtr, sUBaseType_t timeoutTicks);
```
- **@param `queueHandle`:** The handle of the queue.
- **@param `itemPtr`:** A pointer to a buffer to store the received item.
- **@param `timeoutTicks`:** Maximum ticks to wait for an item.
- **@retval `true`:** An item was received.
- **@retval `false`:** Timeout occurred.
- **@warning:** Not safe to call from an ISR.

### `sRTOSQueueSend`
Sends an item to a queue.
```c
sbool_t sRTOSQueueSend(sQueueHandle_t *queueHandle, void *itemPtr, sUBaseType_t timeoutTicks);
```
- **@param `queueHandle`:** The handle of the queue.
- **@param `itemPtr`:** A pointer to the item to be sent.
- **@param `timeoutTicks`:** Maximum ticks to wait for space in the queue.
- **@retval `true`:** The item was sent.
- **@retval `false`:** Timeout occurred.
- **@warning:** Not safe to call from an ISR.

### `sRTOSQueueSendFromISR`
Sends an item to a queue from an ISR.
```c
sbool_t sRTOSQueueSendFromISR(sQueueHandle_t *queueHandle, void *itemPtr);
```
- **@param `queueHandle`:** The handle of the queue.
- **@param `itemPtr`:** A pointer to the item to be sent.
- **@retval `true`:** The item was sent.
- **@retval `false`:** The queue was full.

## Utilities

### `srMS_TO_TICKS`
Converts milliseconds to RTOS ticks.
```c
__STATIC_FORCEINLINE__ sUBaseType_t srMS_TO_TICKS(sUBaseType_t timeoutMS);
```
- **@param `timeoutMS`:** The time in milliseconds.
- **@return:** The equivalent time in RTOS ticks.


## Quick usage

```c
#include <stdint.h>
#include "stm32f4xx.h"
#include "simpleRTOS.h"

uint32_t count0, count1, count2, count3, count4;
uint32_t timercount0, timercount1, timercount2;

sTaskHandle_t Task0H;
sTaskHandle_t Task1H;
sTaskHandle_t Task2H;
sTaskHandle_t Task3H;
sTaskHandle_t Task4H;
sTimerHandle_t Timer0H;
sTimerHandle_t Timer1H;
sTimerHandle_t Timer2H;

void Timer0(sTimerHandle_t *h)
{
  timercount0++;
}
void Timer1(sTimerHandle_t *h)
{
  timercount1++;
}
void Timer2(sTimerHandle_t *h)
{
  timercount2++;
}

void Task0(void *)
{
  while (1)
  {
    count0++;
    if (count0 == 100000)
    {
      sRTOSTaskStop(&Task4H);
    }
  }
}

void Task1(void *)
{
  while (1)
  {
    count1++;
  }
}

void Task2(void *)
{
  while (1)
  {
    count2++;
  }
}

void Task3(void *)
{
  sRTOSTaskDelay(5);
  while (1)
  {
    count3++;
    if ((count3 % 1000) == 0)
    {
      sRTOSTaskDelay(5);
    }
  }
}

void Task4(void *)
{
  sRTOSTaskDelay(10);
  while (1)
  {
    count4++;
    if ((count4 % 1000) == 0)
    {
      sRTOSTaskDelay(5);
    }
  }
}

int main(void)
{
  SystemCoreClockUpdate();
  sRTOSInit(SystemCoreClock);

  sRTOSTaskCreate(Task4,
                  "Task4",
                  NULL,
                  128,
                  sPriorityHigh,
                  &Task4H,
                  sFalse);
  sRTOSTaskCreate(Task3,
                  "Task3",
                  NULL,
                  128,
                  sPriorityHigh,
                  &Task3H,
                  sFalse);
  sRTOSTaskCreate(Task2,
                  "Task2",
                  NULL,
                  128,
                  sPriorityNormal,
                  &Task2H,
                  sTrue);

  sRTOSTaskCreate(Task1,
                  "Task1",
                  NULL,
                  128,
                  sPriorityNormal,
                  &Task1H,
                  sTrue);
  sRTOSTaskCreate(Task0,
                  "Task0",
                  NULL,
                  128,
                  sPriorityNormal,
                  &Task0H,
                  sFalse);

  sRTOSTimerCreate(
      Timer0,
      1,
      80,
      sTrue,
      &Timer0H);

  sRTOSTimerCreate(
      Timer1,
      1,
      160,
      sTrue,
      &Timer1H);
  sRTOSTimerCreate(
      Timer2,
      1,
      5,
      sFalse,
      &Timer2H);

  sRTOSStartScheduler();
}
```
