/*
 * simpleRTOSSemaphore.c
 *
 *  Created on: Aug 17, 2025
 *      Author: benhih
 */

#include "simpleRTOS.h"
#include "stdlib.h"

extern sbool_t _pushTaskNotification(sTaskHandle_t *task, sUBaseType_t message,
                                     sNotificationType_t type, sPriority_t priority);

extern sTaskHandle_t *_sCurrentTask;
extern volatile sUBaseType_t _sTickCount;

void sRTOSSemaphoreCreate(sSemaphore_t *sem, sBaseType_t n)
{
  *sem = n;
}

void sRTOSSemaphoreGive(sSemaphore_t *sem)
{
  __sCriticalRegionBegin();
  (*sem)++;
  __sCriticalRegionEnd();
}

sbool_t sRTOSSemaphoreTake(sSemaphore_t *sem, sUBaseType_t timeoutTicks)
{
  sUBaseType_t timeoutFinish = SAT_ADD_U32(_sTickCount, timeoutTicks);
  __sCriticalRegionBegin();
  while (*sem <= 0)
  {
    __sCriticalRegionEnd();
    if (timeoutFinish <= _sTickCount)
    {
      return srFalse;
    }
    __sCriticalRegionBegin();
  }

  (*sem)--;
  __sCriticalRegionEnd();
  return srTrue;
}

sbool_t sRTOSSemaphoreCooperativeTake(sSemaphore_t *sem, sUBaseType_t timeoutTicks)
{
  sUBaseType_t timeoutFinish = SAT_ADD_U32(_sTickCount, timeoutTicks);
  __sCriticalRegionBegin();
  while (*sem <= 0)
  {
    __sCriticalRegionEnd();
    if (timeoutFinish <= _sTickCount)
    {
      return srFalse;
    }
    sRTOSTaskYield();
    __sCriticalRegionBegin();
  }

  (*sem)--;
  __sCriticalRegionEnd();
  return srTrue;
}

void sRTOSMutexCreate(sMutex_t *mux)
{
  mux->sem = 1;
  mux->holderHandle = NULL;
}

sbool_t sRTOSMutexGive(sMutex_t *mux)
{
  if (mux->sem == 1 || mux->holderHandle != _sCurrentTask)
    return srFalse;

  __sCriticalRegionBegin();
  _pushTaskNotification(mux->requesterHandle, NULL,
                        sNotificationMutex, _sCurrentTask->priority);
  mux->sem++;
  __sCriticalRegionEnd();
  sRTOSTaskYield();
  return srTrue;
}

sbool_t sRTOSMutexGiveFromISR(sMutex_t *mux)
{
  if (mux->sem == 1)
    return srFalse;

  __sCriticalRegionBegin();
  _pushTaskNotification(mux->requesterHandle, NULL,
                        sNotificationMutex, sPriorityMax); // ISRs have a higher priority then any task;
  mux->sem++;
  __sCriticalRegionEnd();
  return srTrue;
}

sbool_t sRTOSMutexTake(sMutex_t *mux, sUBaseType_t timeoutTicks)
{
  sUBaseType_t timeoutFinish = SAT_ADD_U32(_sTickCount, timeoutTicks);
  __sCriticalRegionBegin();
  mux->requesterHandle = _sCurrentTask;
  while (mux->sem <= 0)
  {
    __sCriticalRegionEnd();
    if (timeoutFinish <= _sTickCount)
    {
      return srFalse;
    }
    sRTOSTaskYield();
    __sCriticalRegionBegin();
  }

  mux->holderHandle = _sCurrentTask;
  mux->sem--;
  __sCriticalRegionEnd();
  return srTrue;
}