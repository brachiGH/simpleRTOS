/*
 * simpleRTOS.c
 *
 *  Created on: Aug 14, 2025
 *      Author: brachiGH
 */

#include "simpleRTOSConfig.h"
#include "simpleRTOS.h"
#include "stdlib.h"

sUBaseType_t _sRTOS_timerCount = 0;
sTimerHandle_t *_sRTOS_timerLIST[__sTIMER_LIST_LENGTH] = {NULL};
extern sUBaseType_t _sIsTimerRunning;

__STATIC_FORCEINLINE__ void _timerReturn(void *)
{
  _sIsTimerRunning = 0;
  sRTOSTaskYield();
}

__STATIC_NAKED__ void _timerStart(sTimerHandle_t *, sTimerFunc_t timerTask)
{
  __asm volatile(
      "sub  sp, #32 \n" // to keep the register that restore the context unchanged and reusable
      "bx   r1      \n" // timerTask is the second argument thus stored in r1
      ::: "memory");
}

static sUBaseType_t *_taskInitStack(sTimerFunc_t timerFunc, sTimerHandle_t *arg)
{
  sUBaseType_t stacksize = CONTEXT_STACK_SIZE + __sTIMER_TASK_STACK_DEPTH;
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

  stack[stacksize - 8] = (sUBaseType_t)arg;            // R0
  stack[stacksize - 7] = (sUBaseType_t)(timerFunc);    // R1
  stack[stacksize - 3] = (sUBaseType_t)(_timerReturn); // LR
  // The task address is set in the PC register
  stack[stacksize - 2] = (sUBaseType_t)(_timerStart); // PC
  // set to Thumb mode
  stack[stacksize - 1] = 0x01000000; // xPSR

#ifdef DEBUG
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

sTimerHandle_t *_GetTimer(sUBaseType_t _sIsTimerRunning)
{
  register sBaseType_t minTick = 1;
  register size_t minTickIdex;
  for (register size_t i = 0; i < __sTIMER_LIST_LENGTH; i++)
  {
    if (_sRTOS_timerLIST[i] != NULL && _sRTOS_timerLIST[i]->status == sReady)
    {
      _sRTOS_timerLIST[i]->ticksElapsed--;
      if (minTick > _sRTOS_timerLIST[i]->ticksElapsed)
      {
        minTick = _sRTOS_timerLIST[i]->ticksElapsed;
        minTickIdex = i;
      }
    }
  }

  if (!_sIsTimerRunning && minTick <= 0)
  {
    if (_sRTOS_timerLIST[minTickIdex]->autoReload == srFalse)
      _sRTOS_timerLIST[minTickIdex]->status = sBlocked;

    _sRTOS_timerLIST[minTickIdex]->ticksElapsed = _sRTOS_timerLIST[minTickIdex]->Period;
    return _sRTOS_timerLIST[minTickIdex];
  }

  return NULL;
}

sRTOS_StatusTypeDef _insterTimer(sTimerHandle_t *timerHandle)
{
  sUBaseType_t i = 0;
  while (_sRTOS_timerLIST[i] != NULL)
  {
    if (i == __sTIMER_LIST_LENGTH)
    {
      return sRTOS_TIMER_LIST_IS_FULL;
    }
    i++;
  }

  _sRTOS_timerLIST[i] = timerHandle;
  _sRTOS_timerCount++;
  return sRTOS_OK;
}

void _deleteTimer(sTimerHandle_t *timerHandle)
{
  sUBaseType_t i = 1;
  while (_sRTOS_timerLIST[i] != timerHandle)
  {
    if (i == __sTIMER_LIST_LENGTH)
    {
      return;
    }
    i++;
  }

  _sRTOS_timerLIST[i] = NULL;
  _sRTOS_timerCount--;
  return;
}

sRTOS_StatusTypeDef sRTOSTimerCreate(
    sTimerFunc_t timerTask,
    sUBaseType_t id,
    sBaseType_t period,
    sUBaseType_t autoReload,
    sTimerHandle_t *timerHandle)
{
  // using this function thier is 7 words added to save r4-r11
  sUBaseType_t *stack = _taskInitStack(timerTask, (void *)timerHandle);
  if (stack == NULL)
    return sRTOS_ALLOCATION_FAILED;

  timerHandle->stackPt = (sUBaseType_t *)(stack + __sTIMER_TASK_STACK_DEPTH); // no need to restore r4-r11
  timerHandle->stackBase = (sUBaseType_t *)stack;
  timerHandle->id = id;
  timerHandle->Period = period;
  timerHandle->ticksElapsed = period;
  timerHandle->autoReload = (sbool_t)autoReload;
  timerHandle->status = sReady;

  return _insterTimer(timerHandle);
}

// Stopping while the timer is still running will not stop it immediately.
// stop will prevent the timer from running again, until resumed
void sRTOSTimerStop(sTimerHandle_t *timerHandle)
{
  timerHandle->status = sBlocked;
}
void sRTOSTimerResume(sTimerHandle_t *timerHandle)
{
  timerHandle->status = sReady;
}

// If a NULL or invalid timerHandle is provided, no action is taken.
// If a timer deletes itself or is deleted from an ISR,
// the timer continues executing until it returns.
// Deleting while the timer is still running causes use-after-free,
// Do NOT delete a timer until the timer is no longer in use (Because it memory is freed).
void sRTOSTimerDelete(sTimerHandle_t *timerHandle)
{
  _deleteTimer(timerHandle);
  free(timerHandle->stackBase);
  free(timerHandle);
}

void sRTOSTimerUpdatePeriode(sTimerHandle_t *timerHandle, sBaseType_t period)
{
  timerHandle->Period = period;
}
