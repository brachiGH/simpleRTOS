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

extern sUBaseType_t *_taskInitStack(sTaskFunc_t taskFunc, void *arg,
                                    sUBaseType_t stacksize, sTaskFunc_t returnFunc, sUBaseType_t fpsMode);

__STATIC_NAKED void _timerReturn(void *)
{
  _sIsTimerRunning = 0;
  sRTOSTaskYield();
}

sTimerHandle_t *_GetTimer(sUBaseType_t _sIsTimerRunning)
{
  register sUBaseType_t minTick;
  register sUBaseType_t minTickIdex;
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
    sUBaseType_t period,
    sUBaseType_t autoReload,
    sTimerHandle_t *timerHandle)
{
  // using this function thier is 7 words added to save r4-r11
  sUBaseType_t *stack = _taskInitStack((sTaskFunc_t)timerTask,
                                       (void *)timerHandle, __sTIMER_TASK_STACK_DEPTH,
                                        _timerReturn, srFalse);
  if (stack == NULL)
    return sRTOS_ALLOCATION_FAILED;

  timerHandle->stackPt = (sUBaseType_t *)(stack + __sTIMER_TASK_STACK_DEPTH - 7); // no need to restore r4-r11
  timerHandle->stackBase = (sUBaseType_t *)stack;
  timerHandle->id = id;
  timerHandle->Period = period;
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

void sRTOSTimerUpdatePeriode(sTimerHandle_t *timerHandle, sUBaseType_t period)
{
  timerHandle->Period = period;
}
