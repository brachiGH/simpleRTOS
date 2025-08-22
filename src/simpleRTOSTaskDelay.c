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
volatile sUBaseType_t __EarliestDelayUptime = 0;

typedef struct simpleRTOSDelay
{
  sTaskHandle_t *task;
  sTimerHandle_t *timer;
  sUBaseType_t dontRunUntil; // time in ticks where the task can start running
  struct simpleRTOSDelay *next;
} simpleRTOSDelay;

simpleRTOSDelay *__delayList = NULL;

void __insertDelay(simpleRTOSDelay *delay)
{
  __sCriticalRegionBegin();
  if (__delayList == NULL)
  {
    __delayList = delay;
    __EarliestDelayUptime = delay->dontRunUntil;
    __sCriticalRegionBegin();
    return;
  }

  if (delay->dontRunUntil < __delayList->dontRunUntil)
  {
    __EarliestDelayUptime = delay->dontRunUntil;
    delay->next = __delayList;
    __delayList = delay;
    __sCriticalRegionBegin();
    return;
  }

  simpleRTOSDelay *curr = __delayList;
  while (curr->next && curr->next->dontRunUntil < delay->dontRunUntil)
  {
    curr = curr->next;
  }

  delay->next = curr->next;
  curr->next = delay;
  __sCriticalRegionBegin();
}

simpleRTOSDelay *__popFirstDelay(void)
{
  if (__delayList == NULL)
  {
    return NULL;
  }

  simpleRTOSDelay *first = __delayList;
  __delayList = first->next;

  __EarliestDelayUptime = __delayList->dontRunUntil;
  return first;
}

void _removeTimerDelayList(sTimerHandle_t *timer)
{
  __sCriticalRegionBegin();
  if (__delayList == NULL)
  {
    __sCriticalRegionEnd();
    return;
  }

  if (__delayList->timer == timer)
  {
    simpleRTOSDelay *temp = __delayList;
    __delayList = __delayList->next;
    __sCriticalRegionEnd();
    free(temp);
    return;
  }

  simpleRTOSDelay *curr = __delayList;
  while (curr->next && curr->next->timer != timer)
  {
    curr = curr->next;
  }

  simpleRTOSDelay *temp = curr->next;
  curr->next = temp->next;
  __sCriticalRegionEnd();
  free(temp);
}

void _removeTaskDelayList(sTaskHandle_t *task)
{
  __sCriticalRegionBegin();
  if (__delayList == NULL)
  {
    __sCriticalRegionEnd();
    return;
  }

  if (__delayList->task == task)
  {
    simpleRTOSDelay *temp = __delayList;
    __delayList = __delayList->next;
    __sCriticalRegionEnd();
    free(temp);
    return;
  }

  simpleRTOSDelay *curr = __delayList;
  while (curr->next && curr->next->task != task)
  {
    curr = curr->next;
  }

  simpleRTOSDelay *temp = curr->next;
  curr->next = temp->next;
  __sCriticalRegionEnd();
  free(temp);
}

// function check for tasks and timer that are done wainting
// it re-insert task that are done back to the ready taskList
// for timer it re-insert them into the __delayList if autoReload is on,
// and then it returns them to tell the scheduler a time is ready to run
void *_sCheckDelays(void)
{
  while (__delayList != NULL && __EarliestDelayUptime <= _sTickCount)
  {
    __sCriticalRegionBegin();
    simpleRTOSDelay *doneDelay = __popFirstDelay();
    if (doneDelay->task != NULL)
    {
      _insertTask(doneDelay->task);
      // _insertTask will exit critical region that why the __sCriticalRegionBegin(); is called in the loop
      free(doneDelay);
    }
    else
    {
      if (doneDelay->timer->autoReload == sTrue)
      {
        doneDelay->dontRunUntil = SAT_ADD_U32(doneDelay->dontRunUntil, doneDelay->timer->Period);
        __insertDelay(doneDelay);
        // __insertDelay will exit critical region
      }
      else
      {
        __sCriticalRegionEnd();
        free(doneDelay);
      }

      return doneDelay->timer;
    }
  }

  return NULL; // returning null means no timer to run
}

// only works on task not timers
void sRTOSTaskDelay(sUBaseType_t duration_ms)
{
  simpleRTOSDelay *delay = (simpleRTOSDelay *)malloc(sizeof(simpleRTOSDelay));

  delay->task = _sCurrentTask;
  delay->dontRunUntil = SAT_ADD_U32(_sTickCount, (duration_ms * (__sRTOS_SENSIBILITY / 1000)));
  delay->next = NULL;

  __sCriticalRegionBegin();
  _sCurrentTask->status = sWaiting;
  _deleteTask(_sCurrentTask, sFalse);
  __insertDelay(delay);
  __sCriticalRegionEnd();
  sRTOSTaskYield();
}
