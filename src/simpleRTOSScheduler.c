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

extern sTaskNotification_t *_popHighestPriorityNotif();

/*************PV*****************/
sTaskHandle_t *_sRTOS_TaskList;
sTaskHandle_t *_sRTOS_IdleTask;
extern sUBaseType_t volatile _sTickCount;
/********************************/

void _idle(void *)
{
  for (;;)
  {
    sRTOSTaskYield();
  }
}

void _insertTask(sTaskHandle_t *task)
{
  __sCriticalRegionBegin();
  // insert as first
  if (_sRTOS_TaskList->priority < task->priority)
  {
    task->nextTask = _sRTOS_TaskList;
    _sRTOS_TaskList = task;
    return;
  }

  sTaskHandle_t *currentTask = _sRTOS_TaskList;
  while (currentTask->nextTask)
  {
    sTaskHandle_t *nextTask = currentTask->nextTask;
    if (nextTask->priority < task->priority)
    {
      task->nextTask = nextTask;
      currentTask->nextTask = task;
      return;
    }
    currentTask = nextTask;
  }

  currentTask->nextTask = task;
  task->nextTask = NULL;
  __sCriticalRegionEnd();
}

void _deleteTask(sTaskHandle_t *task)
{
  __sCriticalRegionBegin();
  if (_sRTOS_TaskList == task)
  {
    _sRTOS_TaskList = _sRTOS_TaskList->nextTask;
    goto freeTask;
  }

  sTaskHandle_t *currentTask = _sRTOS_TaskList;
  while (currentTask->nextTask)
  {
    sTaskHandle_t *nextTask = currentTask->nextTask;
    if (nextTask == task)
    {
      currentTask->nextTask = nextTask->nextTask;
      goto freeTask;
    }
    currentTask = nextTask;
  }

freeTask:
  free(task->stackPt);
  free(task);
  __sCriticalRegionEnd();
  return;
}

sRTOS_StatusTypeDef sRTOSInit(sUBaseType_t BUS_FREQ)
{
  uint32_t PRESCALER = (BUS_FREQ / __sRTOS_SENSIBILITY);

  if (PRESCALER == 0) return sRTOS_ERROR; // avoid underflow

  SYST_CSR = 0;             // disable SysTick
  SYST_CVR = 0;             // clear current value
  SYST_RVR = ((PRESCALER) - 1) & 0x00FFFFFFu; // RVR is 24-bit

  /* set PendSV lowest, SysTick just above PendSV (use byte access to avoid endian/shift mistakes) */
  uint8_t *shpr3 = (uint8_t *)&SYSPRI3;
  shpr3[2] = 0xFFu; // PendSV priority byte
  shpr3[3] = 0xFEu; // SysTick priority byte

  SYST_CSR = 0x00000007; // enable SysTick: ENABLE | TICKINT | CLKSOURCE


  _sRTOS_IdleTask = (sTaskHandle_t *)malloc(sizeof(sTaskHandle_t));
  if (_sRTOS_IdleTask == NULL)
  {
    return sRTOS_ALLOCATION_FAILED;
  }
  _sRTOS_TaskList = _sRTOS_IdleTask; // idle should recall it self when thier is no other task to do
  return sRTOSTaskCreate(_idle,
                         "idle task",
                         NULL,
                         4,
                         sPriorityIdle,
                         _sRTOS_IdleTask,
                         srFALSE);
}

sTaskHandle_t *_sRTOSGetFirstAvailableTask(void)
{
  sTaskHandle_t *task = _sRTOS_TaskList;
  sTaskNotification_t *taskWithNotificataion = _popHighestPriorityNotif();
  while (task)
  {
    if ((task->status == sReady || task->status == sRunning) && task->dontRunUntil <= _sTickCount)
    {
      if (taskWithNotificataion && taskWithNotificataion->priority >= task->priority)
      {
        if (taskWithNotificataion->type == sNotificationMutex)
        {
          // When Mutex sends a notification it is not sending messages
          free(taskWithNotificataion); // this causes a after free but it is fine because this function is
                                       // run in a critical region (it won't change)
        }

        return taskWithNotificataion->task;
      }
      return task;
    }
    task = task->nextTask;
  }

  return _sRTOS_IdleTask;
}
