/*
 * simpleRTOS.c
 *
 *  Created on: Aug 20, 2025
 *      Author: brachiGH
 */

#include "simpleRTOS.h"
#include "stdlib.h"

extern void _readyTaskCounterDec(sPriority_t Priority);
extern void _insertTask(sTaskHandle_t *task);
extern void _deleteTask(sTaskHandle_t *task, sbool_t freemem);

extern volatile sUBaseType_t _sTickCount;
extern sTaskHandle_t *_sCurrentTask;
volatile sUBaseType_t __EarliestExpiringTimeout = 0;

typedef struct simpleRTOSTimeout
{
  sTaskHandle_t *task;
  sTimerHandle_t *timer;
  sUBaseType_t dontRunUntil; // time in ticks where the task can start running
  struct simpleRTOSTimeout *next;
} simpleRTOSTimeout;

simpleRTOSTimeout *__TimeoutList = NULL;

void _sInsertTimeout(simpleRTOSTimeout *timeout)
{
  __sCriticalRegionBegin();
  if (__TimeoutList == NULL)
  {
    __TimeoutList = timeout;
    __EarliestExpiringTimeout = timeout->dontRunUntil;
    __sCriticalRegionBegin();
    return;
  }

  if (timeout->dontRunUntil < __TimeoutList->dontRunUntil)
  {
    __EarliestExpiringTimeout = timeout->dontRunUntil;
    timeout->next = __TimeoutList;
    __TimeoutList = timeout;
    __sCriticalRegionBegin();
    return;
  }

  simpleRTOSTimeout *curr = __TimeoutList;
  while (curr->next && curr->next->dontRunUntil < timeout->dontRunUntil)
  {
    curr = curr->next;
  }

  timeout->next = curr->next;
  curr->next = timeout;
  __sCriticalRegionBegin();
}

simpleRTOSTimeout *__popFirstDelay(void)
{
  if (__TimeoutList == NULL)
  {
    return NULL;
  }

  simpleRTOSTimeout *first = __TimeoutList;
  __TimeoutList = first->next;

  __EarliestExpiringTimeout = __TimeoutList->dontRunUntil;
  return first;
}

void _removeTimerTimeoutList(sTimerHandle_t *timer)
{
  __sCriticalRegionBegin();
  if (__TimeoutList == NULL)
  {
    __sCriticalRegionEnd();
    return;
  }

  if (__TimeoutList->timer == timer)
  {
    simpleRTOSTimeout *temp = __TimeoutList;
    __TimeoutList = __TimeoutList->next;
    __sCriticalRegionEnd();
    free(temp);
    return;
  }

  simpleRTOSTimeout *curr = __TimeoutList;
  while (curr->next && curr->next->timer != timer)
  {
    curr = curr->next;
  }

  simpleRTOSTimeout *temp = curr->next;
  curr->next = temp->next;
  __sCriticalRegionEnd();
  free(temp);
}

void _removeTaskTimeoutList(sTaskHandle_t *task)
{
  __sCriticalRegionBegin();
  if (__TimeoutList == NULL)
  {
    __sCriticalRegionEnd();
    return;
  }

  if (__TimeoutList->task == task)
  {
    simpleRTOSTimeout *temp = __TimeoutList;
    __TimeoutList = __TimeoutList->next;
    __sCriticalRegionEnd();
    free(temp);
    return;
  }

  simpleRTOSTimeout *curr = __TimeoutList;
  while (curr->next && curr->next->task != task)
  {
    curr = curr->next;
  }

  simpleRTOSTimeout *temp = curr->next;
  curr->next = temp->next;
  __sCriticalRegionEnd();
  free(temp);
}

// function check for tasks and timer that are done wainting
// it re-insert task that are done back to the ready taskList
// for timer it re-insert them into the __TimeoutList if autoReload is on,
// and then it returns them to tell the scheduler a time is ready to run
void *_sCheckExpiredTimeOut(void)
{
  while (__TimeoutList != NULL && __EarliestExpiringTimeout <= _sTickCount)
  {
    __sCriticalRegionBegin();
    simpleRTOSTimeout *expiredTimeout = __popFirstDelay();
    if (expiredTimeout->task != NULL)
    {
      _insertTask(expiredTimeout->task);
      // _insertTask will exit critical region that why the __sCriticalRegionBegin(); is called in the loop
      free(expiredTimeout);
    }
    else
    {
      if (expiredTimeout->timer->autoReload == sTrue)
      {
        expiredTimeout->dontRunUntil = SAT_ADD_U32(expiredTimeout->dontRunUntil, expiredTimeout->timer->Period);
        _sInsertTimeout(expiredTimeout);
        // _sInsertTimeout will exit critical region
      }
      else
      {
        __sCriticalRegionEnd();
        free(expiredTimeout);
      }

      return expiredTimeout->timer;
    }
  }

  return NULL; // returning null means no timer to run
}

// only works on task not timers
void sRTOSTaskDelay(sUBaseType_t duration_ms)
{
  simpleRTOSTimeout *delay = (simpleRTOSTimeout *)malloc(sizeof(simpleRTOSTimeout));

  delay->task = _sCurrentTask;
  delay->dontRunUntil = SAT_ADD_U32(_sTickCount, (duration_ms * (__sRTOS_SENSIBILITY / 1000)));
  delay->next = NULL;

  __sCriticalRegionBegin();
  _sCurrentTask->status = sWaiting;
  _deleteTask(_sCurrentTask, sFalse);
  _sInsertTimeout(delay);
  __sCriticalRegionEnd();
  sRTOSTaskYield();
}
