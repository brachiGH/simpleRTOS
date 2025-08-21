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

typedef struct simpleRTOSTaskDelay
{
  sTaskHandle_t *task;
  sUBaseType_t dontRunUntil; // time in ticks where the task can start running
  struct simpleRTOSTaskDelay *next;
} simpleRTOSTaskDelay;

simpleRTOSTaskDelay *__delayList = NULL;

void __insertDelay(simpleRTOSTaskDelay *delay)
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

  simpleRTOSTaskDelay *curr = __delayList;
  while (curr->next && curr->next->dontRunUntil <= delay->dontRunUntil)
  {
    curr = curr->next;
  }

  delay->next = curr->next;
  curr->next = delay;
}

simpleRTOSTaskDelay *__popFirstDelay(void)
{
  if (__delayList == NULL)
  {
    return NULL;
  }

  simpleRTOSTaskDelay *first = __delayList;
  __delayList = first->next;
  first->next = NULL;

  __EarliestDelayUptime = first->next->dontRunUntil;
  return first;
}

void _sCheckDelays(void)
{
  while (__delayList != NULL && __EarliestDelayUptime <= _sTickCount)
  {
    simpleRTOSTaskDelay *temp = __popFirstDelay();
    _insertTask(temp->task);
    free(temp);
  }
}

// only works on task not timers
void sRTOSTaskDelay(sUBaseType_t duration_ms)
{
  simpleRTOSTaskDelay *delay = (simpleRTOSTaskDelay *)malloc(sizeof(simpleRTOSTaskDelay));

  delay->task = _sCurrentTask;
  delay->dontRunUntil = SAT_ADD_U32(_sTickCount, (duration_ms * (__sRTOS_SENSIBILITY / 1000)));
  delay->next = NULL;

  __sCriticalRegionBegin();
  __insertDelay(delay);
  sRTOSTaskStop(_sCurrentTask); // stop task yeilds when stoping the current task
  __sCriticalRegionEnd();
}
