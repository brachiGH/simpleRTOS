/*
 * simpleRTOSTaskNotication.c
 *
 *  Created on: Aug 19, 2025
 *      Author: brachigh
 */

#include "simpleRTOS.h"
#include "stdlib.h"

extern void _deleteTask(sTaskHandle_t *task, sbool_t freeMem);

extern sTaskHandle_t *_sCurrentTask;
extern volatile sUBaseType_t _sTickCount;

void _pushTaskNotification(sTaskHandle_t *task, sUBaseType_t *message, sPriority_t priority)
{
  __sCriticalRegionBegin();
  if (message == NULL)
  {
    task->hasNotification = sTrue;
    task->notificationMessage = *message;
  }
  if (task->priority < priority)
  {
    task->priority = priority;
    _deleteTask(task, sFalse);
    _insertTask(task);
  }
  __sCriticalRegionEnd();
}

void sRTOSTaskNotify(sTaskHandle_t *taskToNotify, sUBaseType_t message)
{
  _pushTaskNotification(taskToNotify, (void *)message, _sCurrentTask->priority);
}

sUBaseType_t sRTOSTaskNotifyTake(sUBaseType_t timeoutTicks)
{
  sUBaseType_t timeoutFinish = SAT_ADD_U32(_sTickCount, timeoutTicks);
  __sCriticalRegionBegin();
  while (!_sCurrentTask->hasNotification)
  {
    __sCriticalRegionEnd();
    if (timeoutFinish <= _sTickCount)
    {
      return sFalse;
    }
    sRTOSTaskYield();
    __sCriticalRegionBegin();
  }
  _sCurrentTask->hasNotification = sFalse;

  __sCriticalRegionEnd();
  return _sCurrentTask->notificationMessage;
}
