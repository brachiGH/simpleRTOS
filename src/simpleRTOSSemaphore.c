/*
 * simpleRTOSSemaphore.c
 *
 *  Created on: Aug 17, 2025
 *      Author: benhih
 */

#include "simpleRTOS.h"
#include "stdlib.h"

typedef struct _waitingForMutexGive
{
  sTaskHandle_t *task;
  sPriority_t priority;
  struct _waitingForMutexGive *next;
} _waitingForMutexGive;

extern sTaskHandle_t *_sRTOS_CurrentTask;
extern sUBaseType_t volatile _sTickCount;

_waitingForMutexGive *_waitingForMutexGive_Head = NULL;

sTaskHandle_t *_getHighestPriorityMutexWaitingTask(void)
{
  if (_waitingForMutexGive_Head == NULL)
    return NULL;

  _waitingForMutexGive *curr = _waitingForMutexGive_Head;
  sTaskHandle_t *highestTask = NULL;
  sPriority_t highestPriority = sPriorityIdle;

  while (curr)
  {
    if (curr->priority > highestPriority || highestTask == NULL)
    {
      highestPriority = curr->priority;
      highestTask = curr->task;
    }
    curr = curr->next;
  }
  return highestTask;
}

__STATIC_FORCEINLINE__ _waitingForMutexGive *waitingForMutexGive_add(sTaskHandle_t *task, sPriority_t priority)
{
  _waitingForMutexGive *newNode = (_waitingForMutexGive *)malloc(sizeof(_waitingForMutexGive));
  if (!newNode)
    return NULL;
  newNode->task = task;
  newNode->priority = priority;
  newNode->next = NULL;

  if (_waitingForMutexGive_Head == NULL)
  {
    _waitingForMutexGive_Head = newNode;
    return newNode;
  }

  _waitingForMutexGive *curr = _waitingForMutexGive_Head;
  while (curr->next)
  {
    curr = curr->next;
  }
  curr->next = newNode;
  return newNode;
}

__STATIC_FORCEINLINE__ void waitingForMutexGive_delete(sTaskHandle_t *task)
{
  _waitingForMutexGive *curr = _waitingForMutexGive_Head, *prev = NULL;
  while (curr)
  {
    if (curr->task == task)
    {
      if (prev)
        prev->next = curr->next;
      else
        _waitingForMutexGive_Head = curr->next;
      free(curr);
      return;
    }
    prev = curr;
    curr = curr->next;
  }
}

void sRTOSSemaphoreCreate(sSemaphore_t *sem, sBaseType_t n)
{
  *sem = n;
}

void sRTOSSemaphoreGive(sSemaphore_t *sem)
{
  __sCriticalRegionBegin();
  (*sem)++;
  __sCriticalRegionEnc();
}

sbool_t sRTOSSemaphoreTake(sSemaphore_t *sem, sUBaseType_t timeoutTicks)
{
  sUBaseType_t timeoutAfter = SAT_ADD_U32(_sTickCount, timeoutTicks);
  __sCriticalRegionBegin();
  while (*sem <= 0)
  {
    __sCriticalRegionEnc();
    if (_sTickCount >= timeoutAfter)
    {
      return srFalse;
    }
    __sCriticalRegionEnc();
  }

  (*sem)--;
  __sCriticalRegionEnc();
  return srTrue;
}

sbool_t sRTOSSemaphoreCooperativeTake(sSemaphore_t *sem, sUBaseType_t timeoutTicks)
{
  sUBaseType_t timeoutAfter = SAT_ADD_U32(_sTickCount, timeoutTicks);
  __sCriticalRegionBegin();
  while (*sem <= 0)
  {
    __sCriticalRegionEnc();
    if (_sTickCount >= timeoutAfter)
    {
      return srFalse;
    }
    sRTOSTaskYield();
    __sCriticalRegionEnc();
  }

  (*sem)--;
  __sCriticalRegionEnc();
  return srTrue;
}

void sRTOSMutexCreate(sMutex_t *mux)
{
  mux->sem = 1;
  mux->holderHandle = NULL;
}

sbool_t sRTOSMutexGive(sMutex_t *mux)
{
  if (mux->sem == 1 || mux->holderHandle != _sRTOS_CurrentTask)
    return srFalse;

  __sCriticalRegionBegin();
  waitingForMutexGive_add(mux->requesterHandle, _sRTOS_CurrentTask->priority);
  mux->sem++;
  __sCriticalRegionEnc();
  sRTOSTaskYield();
  return srTrue;
}

sbool_t sRTOSMutexGiveFromISR(sMutex_t *mux)
{
  if (mux->sem == 1)
    return srFalse;

  __sCriticalRegionBegin();
  waitingForMutexGive_add(mux->requesterHandle, sPriorityRealtime);
  mux->sem++;
  __sCriticalRegionEnc();
  return srTrue;
}

sbool_t sRTOSMutexTake(sMutex_t *mux, sUBaseType_t timeoutTicks)
{
  sUBaseType_t timeoutAfter = SAT_ADD_U32(_sTickCount, timeoutTicks);
  __sCriticalRegionBegin();
  mux->requesterHandle = _sRTOS_CurrentTask;
  while (mux->sem <= 0)
  {
    __sCriticalRegionEnc();
    if (_sTickCount >= timeoutAfter)
    {
      return srFalse;
    }
    sRTOSTaskYield();
    __sCriticalRegionEnc();
  }

  mux->holderHandle = _sRTOS_CurrentTask;
  waitingForMutexGive_delete(_sRTOS_CurrentTask);
  mux->sem--;
  __sCriticalRegionEnc();
  return srTrue;
}