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
  if (__delayList == NULL)
  {
    __delayList = delay;
    __EarliestDelayUptime = delay->dontRunUntil;
    return;
  }

  if (delay->dontRunUntil < __delayList->dontRunUntil)
  {
    __EarliestDelayUptime = delay->dontRunUntil;
    delay->next = __delayList;
    __delayList = delay;
    return;
  }

  simpleRTOSDelay *curr = __delayList;
  while (curr->next && curr->next->dontRunUntil <= delay->dontRunUntil)
  {
    curr = curr->next;
  }

  delay->next = curr->next;
  curr->next = delay;
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
  if (__delayList == NULL)
  {
    return ;
  }

  if (__delayList->timer == timer)
  {
    __delayList = __delayList->next;
    return ;
  }

  simpleRTOSDelay *curr = __delayList;
  while (curr->next && curr->next->timer != timer)
  {
    curr = curr->next;
  }
 
  simpleRTOSDelay *temp = curr->next;
  curr->next = temp->next;
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
    simpleRTOSDelay *doneDelay = __popFirstDelay();
    if (doneDelay->task != NULL)
    {
      _insertTask(doneDelay->task);
      free(doneDelay);
    }
    else
    {
      if (doneDelay->timer->autoReload == srTrue)
      {
        doneDelay->dontRunUntil = SAT_ADD_U32(doneDelay->dontRunUntil, doneDelay->timer->Period);
        __insertDelay(doneDelay);
      } else {
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
  __insertDelay(delay);
  sRTOSTaskStop(_sCurrentTask); // stop task yeilds when stoping the current task
  __sCriticalRegionEnd();
}
