/*
 * simpleRTOS.c
 *
 *  Created on: Aug 14, 2025
 *      Author: brachiGH
 */

#include "simpleRTOS.h"
#include "stdlib.h"

typedef struct simpleRTOSDelay
{
  sTaskHandle_t *task; // set null
  sTimerHandle_t *timer;
  sUBaseType_t dontRunUntil; // time in ticks where the timer can start running
  struct simpleRTOSDelay *next;
} simpleRTOSDelay;

extern void __insertDelay(simpleRTOSDelay *delay);
extern void _removeTimerDelayList(sTimerHandle_t *timer);

extern volatile sUBaseType_t _sIsTimerRunning;
extern sUBaseType_t _sTickCount;

__STATIC_FORCEINLINE__ void _timerReturn(void *)
{
  _sIsTimerRunning = 0;
  __asm volatile(
      "cpsie  i\n"
      "svc    #1");
}

__STATIC_NAKED__ void _timerStart(sTimerHandle_t *, sTimerFunc_t timerTask)
{
  __asm volatile(
      "sub  sp, #32 \n" // to keep the register that restore the context unchanged and reusable
      "bx   r1      \n" // timerTask is the second argument thus stored in r1
      ::: "memory");
}

void __insertTimer(sTimerHandle_t *timerHandle)
{
  simpleRTOSDelay *delay = (simpleRTOSDelay *)malloc(sizeof(simpleRTOSDelay));

  delay->task = NULL;
  delay->timer = timerHandle;
  delay->dontRunUntil = SAT_ADD_U32(_sTickCount, timerHandle->Period);
  delay->next = NULL;

  __insertDelay(delay);
}

static sUBaseType_t *__taskInitStackTimer(sTimerFunc_t timerFunc, sTimerHandle_t *arg)
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

sRTOS_StatusTypeDef sRTOSTimerCreate(
    sTimerFunc_t timerTask,
    sUBaseType_t id,
    sBaseType_t period,
    sUBaseType_t autoReload,
    sTimerHandle_t *timerHandle)
{
  // using this function thier is 7 words added to save r4-r11
  sUBaseType_t *stack = __taskInitStackTimer(timerTask, (void *)timerHandle);
  if (stack == NULL)
    return sRTOS_ALLOCATION_FAILED;

  timerHandle->stackPt = (sUBaseType_t *)(stack + __sTIMER_TASK_STACK_DEPTH); // no need to restore r4-r11
  timerHandle->stackBase = (sUBaseType_t *)stack;
  timerHandle->id = id;
  timerHandle->Period = period;
  timerHandle->autoReload = (sbool_t)autoReload;
  timerHandle->status = sReady;

  __insertTimer(timerHandle);
  return sRTOS_OK;
}

// Stopping while the timer is still running will not stop it immediately.
// stop will prevent the timer from running again, until resumed
void sRTOSTimerStop(sTimerHandle_t *timerHandle)
{
  if (timerHandle->status == sReady)
  {
    __sCriticalRegionBegin();
    timerHandle->status = sBlocked;
    _removeTimerDelayList(timerHandle);
  }
}

void sRTOSTimerResume(sTimerHandle_t *timerHandle)
{
  if (timerHandle->status == sBlocked)
  {
    __sCriticalRegionBegin();
    timerHandle->status = sReady;
    __insertTimer(timerHandle);
  }
}

// If a NULL or invalid timerHandle is provided, no action is taken.
// If a timer deletes itself or is deleted from an ISR,
// the timer continues executing until it returns.
// Deleting while the timer is still running causes use-after-free,
// Do NOT delete a timer until the timer is no longer in use (Because it memory is freed).
void sRTOSTimerDelete(sTimerHandle_t *timerHandle)
{
  _removeTimerDelayList(timerHandle);
  free(timerHandle->stackBase);
}

void sRTOSTimerUpdatePeriod(sTimerHandle_t *timerHandle, sBaseType_t period)
{
  __sCriticalRegionBegin();
  timerHandle->Period = period;
  _removeTimerDelayList(timerHandle);
  __insertTimer(timerHandle);
}
