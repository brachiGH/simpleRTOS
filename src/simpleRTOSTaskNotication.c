/*
 * simpleRTOSTaskNotication.c
 *
 *  Created on: Aug 19, 2025
 *      Author: brachigh
 */

#include "simpleRTOS.h"
#include "stdlib.h"

extern void _deleteTask(sTaskHandle_t *task, sbool_t freeMem);
extern void _insertTask(sTaskHandle_t *task);

extern sTaskHandle_t *_sCurrentTask;

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

void sRTOSTaskNotifyFromISR(sTaskHandle_t *taskToNotify, sUBaseType_t message)
{
  _pushTaskNotification(taskToNotify, (void *)message, sPriorityMax);
}

sUBaseType_t sRTOSTaskNotifyTake(sUBaseType_t timeoutTicks)
{
  sUBaseType_t timeoutFinish = SAT_ADD_U32(sGetTick(), timeoutTicks);
  __sCriticalRegionBegin();
  while (!_sCurrentTask->hasNotification)
  {
    __sCriticalRegionEnd();
    if (timeoutFinish <= sGetTick())
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
