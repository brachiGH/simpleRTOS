/*
 * simpleRTOS.c
 *
 *  Created on: Aug 14, 2025
 *      Author: brachiGH
 */

#include "simpleRTOS.h"
#include "stdlib.h"
#include "string.h"

#define SYST_CSR (*((volatile uint32_t *)0xE000E010))
#define SYST_RVR (*((volatile uint32_t *)0xE000E014))
#define SYST_CVR (*((volatile uint32_t *)0xE000E018))
#define SYST_CALIB (*((volatile uint32_t *)0xE000E01C))

#define SYSPRI3 (*((volatile uint32_t *)0xE000ED20))

/*************PV*****************/
volatile sUBaseType_t __TaskPriorityBitMap = 0x0; // each bit represent a priority if set to 1 then thier are tasks to execute with that priority
sTaskHandle_t *_sTaskList[MAX_TASK_PRIORITY_COUNT] = {NULL};
sUBaseType_t _sNumberOfReadyTaskPerPriority[MAX_TASK_PRIORITY_COUNT] = {0};
sTaskHandle_t *__IdleTask;
volatile sUBaseType_t _sTicksPassedExecutingCurrentTask = __sQUANTA; // set to __sQUANTA so the scheduler can begin without waiting for a quantum of time to pass

sTaskHandle_t *_sCurrentTask;
/********************************/

void _idle(void *)
{
  for (;;)
  {
    sRTOSTaskYield();
  }
}

void _readyTaskCounterInc(sPriority_t priority)
{
  sUBaseType_t priorityIndex = priority + (MAX_TASK_PRIORITY_COUNT / 2); // MAX_TASK_PRIORITY_COUNT/2 is because the priority start from -16 to 15
  _sNumberOfReadyTaskPerPriority[priorityIndex]++;                       // count the number of tasks for each priority
  __TaskPriorityBitMap |= 1u << priorityIndex;                         // set correspanding bit to 1 to tell the scheduler thier is a task to execute
}

void __readyTaskCounterDec(sPriority_t priority)
{
  sUBaseType_t priorityIndex = priority + (MAX_TASK_PRIORITY_COUNT / 2);

  _sNumberOfReadyTaskPerPriority[priorityIndex]--;
  if (_sNumberOfReadyTaskPerPriority[priorityIndex] == 0)
  {
    __TaskPriorityBitMap &= ~(1u << priorityIndex); // set correspanding bit to 0 to tell the scheduler thier is no task to execute
  }
}

// task will always be inserted in the first position.
void _insertTask(sTaskHandle_t *task)
{
  __sCriticalRegionBegin();
  sPriority_t priority = task->priority;
  sUBaseType_t priorityIndex = priority + (MAX_TASK_PRIORITY_COUNT / 2); // MAX_TASK_PRIORITY_COUNT/2 is because the priority start from -16 to 15
  _readyTaskCounterInc(priority);

  sTaskHandle_t *head = _sTaskList[priorityIndex];
  if (head == NULL)
  {
    task->nextTask = task;
    task->prevTask = task;
    _sTaskList[priorityIndex] = task;
    __sCriticalRegionEnd();
    return;
  }

  sTaskHandle_t *tail = head->prevTask;

  // insert at first position
  task->nextTask = head;
  task->prevTask = tail;
  tail->nextTask = task;
  head->prevTask = task;
  _sTaskList[priorityIndex] = task;

  __sCriticalRegionEnd();
  return;
}

void _deleteTask(sTaskHandle_t *task, sbool_t freeMem)
{
  __sCriticalRegionBegin();
  sPriority_t priority = task->priority;
  sUBaseType_t priorityIndex = priority + (MAX_TASK_PRIORITY_COUNT / 2);
  __readyTaskCounterDec(priority);

  sTaskHandle_t *head = _sTaskList[priorityIndex];

  if (head != NULL)
  {
    if (head == task) // only element in list
    {
      _sTaskList[priorityIndex] = NULL;
    }
    else
    {
      // unlink from circular doubly-linked list
      sTaskHandle_t *prev = task->prevTask;
      sTaskHandle_t *next = task->nextTask;
      prev->nextTask = next;
      next->prevTask = prev;

      if (_sTaskList[priorityIndex] == task)
      {
        _sTaskList[priorityIndex] = next; // move head if we removed it
      }
    }
  }

  // clear links to avoid accidental use
  task->nextTask = NULL;
  task->prevTask = NULL;

  __sCriticalRegionEnd();
  if (freeMem)
  {
    free(task->stackBase);
    free(task);
  }
  return;
}

sRTOS_StatusTypeDef sRTOSInit(sUBaseType_t BUS_FREQ)
{
  uint32_t PRESCALER = (BUS_FREQ / __sRTOS_SENSIBILITY);

  if (PRESCALER == 0)
    return sRTOS_ERROR; // avoid underflow

  SYST_CSR = 0;                             // disable SysTick
  SYST_CVR = 0;                             // clear current value
  SYST_RVR = ((PRESCALER)-1) & 0x00FFFFFFu; // RVR is 24-bit

  /* set PendSV lowest, SysTick just above PendSV (use byte access to avoid endian/shift mistakes) */
  uint8_t *shpr3 = (uint8_t *)&SYSPRI3;
  shpr3[2] = 0xF0; // PendSV priority byte
  shpr3[3] = 0xE0; // SysTick priority byte

  __IdleTask = (sTaskHandle_t *)malloc(sizeof(sTaskHandle_t));
  if (__IdleTask == NULL)
  {
    return sRTOS_ALLOCATION_FAILED;
  }
  _sCurrentTask = __IdleTask;
  return sRTOSTaskCreate(_idle,
                         "idle task",
                         NULL,
                         12,
                         sPriorityIdle,
                         __IdleTask,
                         srFALSE);
}

sTaskHandle_t *_sRTOSGetFirstAvailableTask(void)
{

  sUBaseType_t currentPriorityIndex = _sCurrentTask->priority + (MAX_TASK_PRIORITY_COUNT / 2);

  sUBaseType_t priorityIndex; // priorityIndex of what cloud be the next task of execute
  unsigned int leadingZeros = __builtin_clz((unsigned int)__TaskPriorityBitMap);
  priorityIndex = MAX_TASK_PRIORITY_COUNT - (leadingZeros + 1);

  if (
#if __sUSE_PREEMPTION == 1
      _sTicksPassedExecutingCurrentTask >= __sQUANTA // if a quanta has passed then execute another task

      || priorityIndex > currentPriorityIndex // if a higher priority task is ready run it
#else
      1
#endif
  )
  {
    _sTicksPassedExecutingCurrentTask = 0; // rest counter
    sTaskHandle_t *task = _sTaskList[priorityIndex];
    _sTaskList[priorityIndex] = _sTaskList[priorityIndex]->nextTask; // rotate tasks (note that the _sTaskList[priorityIndex] is circular linked list)

    if (task->priority != task->originalPriority)
    {
      // this means that the mutex or notification has change the priority of the task
      task->priority = task->originalPriority;
      _deleteTask(task, sFalse);
      _insertTask(task);
    }
    return task;
  }

  return NULL; // else keep executing current task
}
