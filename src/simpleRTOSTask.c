/*
 * simpleRTOS.c
 *
 *  Created on: Aug 14, 2025
 *      Author: brachiGH
 */

#include "simpleRTOSConfig.h"
#include "simpleRTOS.h"
#include "stdlib.h"
#include "stm32f4xx.h"
#include "string.h"


sTaskHandle_t *_sRTOS_CurrentTask;

extern void _deleteTask(sTaskHandle_t *task);
extern void _insertTask(sTaskHandle_t *task);

void sRTOSTaskYield(void)
{
}

void _taskInitStack(sUBaseType_t *stack, sTaskFunc_t taskFunc, void *arg,
                    sUBaseType_t stacksize)
{
  /*
  Hardware automatically pushes these registers onto the stack (in this order):
      r0
      r1
      r2
      r3
      r12
      lr (return address)
      pc (program counter)
      xPSR (program status register)
*/

  stack[stacksize - 8] = (sUBaseType_t)arg; // R0
  stack[stacksize - 3] = 0xFFFFFFFF; // LR
  // The task address is set in the PC register
  stack[stacksize - 2] = (sUBaseType_t)(taskFunc); // PC
  // set to Thumb mode
  stack[stacksize - 1] = 0x01000000; // xPSR

#ifdef DEBUG
  stack[stacksize - 7] = 0x11111112; // R1
  stack[stacksize - 6] = 0x22222223; // R2
  stack[stacksize - 5] = 0x33333334; // R3
  stack[stacksize - 4] = 0xCCCCCCCE; // R12
  stack[stacksize - 9] = 0x77777778; // r7
  stack[stacksize - 10] = 0x66666667; // r6
  stack[stacksize - 11] = 0x55555556; // r5
  stack[stacksize - 12] = 0x44444445; // r4
  stack[stacksize - 13] = 0xBBBBBBBC; // r11
  stack[stacksize - 14] = 0xAAAAAAAB; // r10
  stack[stacksize - 15] = 0x9999999A; // r9
  stack[stacksize - 16] = 0x88888889; // r8

  // stack[stacksize - 17] = 0xEEEEEEEE; // s31
  // stack[stacksize - 18] = 0xFFFFFFFF; // s32
#endif
}

/*
 * note: can be called after sRTOSStartScheduler()
 * and the new task will be added
 * note: nextTasK changes because of _insertTask
 */
sRTOS_StatusTypeDef sRTOSTaskCreate(
    sTaskFunc_t taskFunc,
    char *name,
    void *arg,
    sUBaseType_t stacksizeWords,
    sPriority_t priority,
    sTaskHandle_t *taskHandle,
    sUBaseType_t fpsMode)
{
  sUBaseType_t minstack = (fpsMode == srFALSE)?  MIN_STACK_SIZE_NO_FPU : MIN_STACK_SIZE_FPU;

  sUBaseType_t *stack = (sUBaseType_t *)malloc(sizeof(sUBaseType_t) * (stacksizeWords + minstack));
  if (stack == NULL)
    return sRTOS_ALLOCATION_FAILED;

  _taskInitStack(stack, taskFunc, arg, stacksizeWords);

  /*
   * Task Init begin
   *
   * Systic (interrupt handler) pushes registers (r4–r11).
   */
  taskHandle->stackPt = (sUBaseType_t *)(stack+ stacksizeWords - minstack); // stack pointer, after stack frame
                                                                     // is saved onto the same stack
  taskHandle->nextTask = NULL;
  taskHandle->status = sReady;
  taskHandle->fps = (sbool_t)fpsMode;
  taskHandle->priority = priority;
  strncpy(taskHandle->name, name, MAX_TASK_NAME_LEN);

  _insertTask(taskHandle);
  return sRTOS_OK;
}

void sRTOSTaskStop(sTaskHandle_t *taskHandle)
{
  if (taskHandle->status != sDeleted)
    taskHandle->status = sBlocked;
}
void sRTOSTaskStart(sTaskHandle_t *taskHandle)
{
  if (taskHandle->status != sDeleted)
    taskHandle->status = sReady;
}

void sRTOSTaskDelete(sTaskHandle_t *taskHandle)
{
  // _deleteTask(taskHandle); // because of the design, deleting the task from
                              // the task list will add a lot of edge cases to
                              // deal with, thus a lot code will be added to the scheduler

  __disable_irq(); // in case deleted task is the next to run
  free(taskHandle->stackPt);// only 26byte are left
  taskHandle->status = sDeleted;
  __enable_irq();

  // if the current task deletes itself yield
  if (taskHandle == _sRTOS_CurrentTask)
    sRTOSTaskYield();
}
