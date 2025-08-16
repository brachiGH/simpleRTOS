/*
 * simpleRTOS.c
 *
 *  Created on: Aug 14, 2025
 *      Author: brachiGH
 */

#include "simpleRTOSConfig.h"
#include "simpleRTOS.h"
#include "stdlib.h"
#include "string.h"


#define SYST_CSR (*((volatile uint32_t *)0xE000E010))
#define SYST_RVR (*((volatile uint32_t *)0xE000E014))
#define SYST_CVR (*((volatile uint32_t *)0xE000E018))
#define SYST_CALIB (*((volatile uint32_t *)0xE000E01C))

#define SYSPRI3 (*((volatile uint32_t *)0xE000ED20))


/*************PV*****************/
sTaskHandle_t *_sRTOS_TaskList;
sTaskHandle_t *_sRTOS_IdleTask;
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
  __sDisable_irq();
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
  __sEnable_irq();
}


void _deleteTask(sTaskHandle_t *task)
{
  __sDisable_irq();
    if (_sRTOS_TaskList == task)
  {
    _sRTOS_TaskList= _sRTOS_TaskList->nextTask;
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
  __sEnable_irq();
  return;
}

sRTOS_StatusTypeDef sRTOSInit(sUBaseType_t BUS_FREQ)
{
  __sDisable_irq();
  uint32_t PRESCALER = (BUS_FREQ / __sRTOS_SENSIBILITY);

  SYST_CSR = 0;                                  // disable Systic timer
  SYST_CVR = 0;                                  // set value to 0
  SYST_RVR = (PRESCALER) - 1; // the value start counting from 0
  /*
   * This makes SysTick the second least urgent interrupt after PendSV,
        (PendSV:
         What it is: a software-triggered, low‑priority exception meant for deferred work. RTOSes use it to perform context switches outside time‑critical ISRs.
         Why it’s used: you can request a switch from any ISR (e.g., SysTick), but the actual save/restore runs in PendSV at the lowest priority so higher‑priority interrupts aren’t delayed.
         How to trigger: set the PENDSVSET bit in SCB->ICSR; hardware clears it on entry.)
   * so it only runs when nothing more important is happening.
   * Other interrupts with higher priority can interrupt the SysTick handler,
   * making your scheduler less likely to interfere with time-critical tasks.
   */
  uint32_t v = SYSPRI3;
  v &= ~(0xFFu << 16 | 0xFFu << 24);   // PendSV: lowest
  v |=  (0xFFu << 16) | (0xFEu << 24); // SysTic: just above PendSV
  SYSPRI3 = v;

  SYST_CSR = 0x00000007; // enable clock + Enables SysTick exception request
                              // + set clock source to processor clock

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

sTaskHandle_t *_sRTOSSchedulerGetFirstAvailable(void)
{
  sTaskHandle_t *task = _sRTOS_TaskList;
  while (task)
  {
    if (task->status == sReady || task->status == sRunning)
    {
      return task;
    }
    task = task->nextTask;
  }

  return _sRTOS_IdleTask;
}
