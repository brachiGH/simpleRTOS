/*
 * simpleRTOSTaskNotication.c
 *
 *  Created on: Aug 19, 2025
 *      Author: brachigh
 */

#include "simpleRTOS.h"
#include "stdlib.h"

extern sTaskHandle_t *_sRTOS_CurrentTask;
extern sUBaseType_t _sTickCount;
sTaskNotification_t *_sRTOSNotifPriorityQueue = NULL;
sbool_t _currentTaskHasNotif = srFalse;

sbool_t _pushTaskNotification(sTaskHandle_t *task, sUBaseType_t message, 
            sNotificationType_t type, sPriority_t priority)
{
  sTaskNotification_t *Notif = malloc(sizeof(sTaskNotification_t));
  if (Notif == NULL) return srFalse;
  
  Notif->task = task;
  Notif->priority = priority;
  Notif->message = message;
  Notif->type = type;
  Notif->next = NULL;

  if (_sRTOSNotifPriorityQueue == NULL)
  {
    _sRTOSNotifPriorityQueue = Notif;
  
    return srTrue;
  }

  if (_sRTOSNotifPriorityQueue->priority <= priority)
  {
    Notif->next = _sRTOSNotifPriorityQueue;
    _sRTOSNotifPriorityQueue = Notif;

    return srTrue;
  }

  sTaskNotification_t *currentNotif = _sRTOSNotifPriorityQueue;
  while (currentNotif->next)
  {
    sTaskNotification_t *nextNotif = currentNotif->next;
    if (nextNotif->priority <= priority)
    {
      currentNotif->next = Notif;
      Notif->next = nextNotif;

      return srTrue;
    }
    currentNotif = nextNotif;
  }

  currentNotif->next = Notif;
  return srTrue;
}

sTaskNotification_t *_checkoutHighestPriorityNotif()
{
  sTaskNotification_t *Notif = _sRTOSNotifPriorityQueue;

  return Notif;
}

// user most free the notification him self
sTaskNotification_t *_popHighestPriorityNotif()
{
  sTaskNotification_t *Notif = _sRTOSNotifPriorityQueue;
  if (Notif == NULL)
    return NULL;

  _sRTOSNotifPriorityQueue = Notif->next;

  return Notif;
}

sbool_t sRTOSTaskNotify(sTaskHandle_t *taskToNotify, sUBaseType_t message)
{
  return _pushTaskNotification(taskToNotify, message, 
            sNotification, _sRTOS_CurrentTask->priority);
}

sUBaseType_t sRTOSTaskNotifyTake(sUBaseType_t timeoutTicks)
{
  sUBaseType_t timeoutFinish = SAT_ADD_U32(_sTickCount, timeoutTicks);
  _sRTOS_CurrentTask->dontRunUntil = timeoutFinish;
  while (!_currentTaskHasNotif)
  {
    sRTOSTaskYield();
  }
  _currentTaskHasNotif = srFalse;

  sTaskNotification_t  *temp =_popHighestPriorityNotif();
  sUBaseType_t msg = temp->message;
  free(temp);
  return msg;
}
