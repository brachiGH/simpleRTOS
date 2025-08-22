/*
 * simpleRTOS.c
 *
 *  Created on: Aug 14, 2025
 *      Author: brachiGH
 */

#include "simpleRTOS.h"
#include "stdlib.h"
#include "string.h"

extern void _deleteTask(sTaskHandle_t *task, sbool_t freeMem);
extern void _insertTask(sTaskHandle_t *task);
extern void _removeTaskTimeoutList(sTaskHandle_t *task);
extern volatile sUBaseType_t _sTickCount;
extern sTaskHandle_t *_sCurrentTask;

__STATIC_NAKED__ void _taskReturn(void *)
{
  for (;;)
  {
  }
}

static sUBaseType_t *_taskInitStack(sTaskFunc_t taskFunc, void *arg,
                                    sUBaseType_t stacksize, sUBaseType_t fpsMode)
{
  stacksize = ((fpsMode == srFALSE) ? MIN_STACK_SIZE_NO_FPU : MIN_STACK_SIZE_FPU) + stacksize;
  sUBaseType_t *stack = (sUBaseType_t *)malloc(sizeof(sUBaseType_t) * (stacksize));
  if (stack == NULL)
    return NULL;

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

  stack[stacksize - 8] = (sUBaseType_t)arg;           // R0
  stack[stacksize - 3] = (sUBaseType_t)(_taskReturn); // LR
  // The task address is set in the PC register
  stack[stacksize - 2] = (sUBaseType_t)(taskFunc); // PC
  // set to Thumb mode
  stack[stacksize - 1] = 0x01000000; // xPSR

#ifdef DEBUG
  stack[stacksize - 7] = 0x11111112;  // R1
  stack[stacksize - 6] = 0x22222223;  // R2
  stack[stacksize - 5] = 0x33333334;  // R3
  stack[stacksize - 4] = 0xCCCCCCCE;  // R12
  stack[stacksize - 9] = 0x77777778;  // r7
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
  return stack;
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
  sUBaseType_t *stack = _taskInitStack(taskFunc, arg, stacksizeWords, fpsMode);
  if (stack == NULL)
    return sRTOS_ALLOCATION_FAILED;

  taskHandle->stackBase = stack;
  /*
   * Task Init begin
   *
   * Systic (interrupt handler) pushes registers (r4–r11).
   */
  taskHandle->stackPt = (sUBaseType_t *)(stack + stacksizeWords); // stack pointer, after stack frame
                                                                  // is saved onto the same stack
  taskHandle->nextTask = taskHandle;                              // if no other task rerun same task
  taskHandle->prevTask = taskHandle;
  taskHandle->status = sReady;
  taskHandle->regitersSaved = sTrue; // Regiters are intialized on the stack by `_taskInitStack`, this means that thier are saved
  taskHandle->fps = (sbool_t)fpsMode;
  taskHandle->priority = priority;
  taskHandle->notificationMessage = 0;
  taskHandle->hasNotification = sFalse;
  taskHandle->originalPriority = priority;
  strncpy(taskHandle->name, name, MAX_TASK_NAME_LEN);

  _insertTask(taskHandle);
  return sRTOS_OK;
}

void sRTOSTaskUpdatePriority(sTaskHandle_t *taskHandle, sPriority_t priority)
{
  __sCriticalRegionBegin();
  taskHandle->priority = priority;
  _deleteTask(taskHandle, sFalse);
  _insertTask(taskHandle);
  // _deleteTask and _insertTask, exit the criticalRegion at the end
}

void sRTOSTaskStop(sTaskHandle_t *taskHandle)
{
  if (taskHandle == NULL)
    taskHandle = _sCurrentTask;

  if (taskHandle->status != sDeleted && taskHandle->status != sBlocked)
  {
    if (taskHandle->status == sWaiting)
    {
      _removeTaskTimeoutList(taskHandle);
    }
    else
    {
      _deleteTask(taskHandle, sFalse); //  this removes the task from the list of ready to execute task but it does not free it memory
                                       // thus we can restore it
    }

    __sCriticalRegionBegin();
    taskHandle->status = sBlocked;
    __sCriticalRegionEnd();

    if (taskHandle == _sCurrentTask)
    {
      sRTOSTaskYield(); // if the current task deletes itself yield
      return;
    }
  }
}

/*
 * will yield if the priority of the current running Task is lower
 * then the resumed Task
 */
void sRTOSTaskResume(sTaskHandle_t *taskHandle)
{
  if (taskHandle->status != sDeleted && taskHandle->status != sReady)
  {
    if (taskHandle->status == sWaiting)
    {
      _removeTaskTimeoutList(taskHandle);
    }
    __sCriticalRegionBegin();
    taskHandle->status = sReady;
    _insertTask(taskHandle);
    // _insertTask, exit the criticalRegion at the end

    if (taskHandle->priority > _sCurrentTask->priority)
    {
      sRTOSTaskYield();
      return;
    }
  }
}

// if provided a none existing taskHandle nothing happens
void sRTOSTaskDelete(sTaskHandle_t *taskHandle)
{
  if (taskHandle == NULL)
    taskHandle = _sCurrentTask;

  _deleteTask(taskHandle, sTrue);

  // if the current task deletes itself yield
  if (taskHandle == _sCurrentTask)
  {
    sRTOSTaskYield();
    return;
  }
}
