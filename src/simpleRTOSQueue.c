/*
 * simpleRTOSTaskNotication.c
 *
 *  Created on: Aug 19, 2025
 *      Author: brachigh
 */

#include "simpleRTOS.h"
#include "stdlib.h"
#include "string.h"

extern volatile sUBaseType_t _sTickCount;
extern sTaskHandle_t *_sCurrentTask;

void sRTOSQueueCreate(sQueueHandle_t *queueHandle, sUBaseType_t queueLengh, sUBaseType_t itemSize)
{
  queueHandle->maxLenght = queueLengh;
  queueHandle->lenght = 0;
  queueHandle->itemSize = itemSize;
  queueHandle->index = -1;
  queueHandle->items = malloc(queueLengh * sizeof(void *));
}
sbool_t sRTOSQueueReceive(sQueueHandle_t *queueHandle, void *itemPtr, sUBaseType_t timeoutTicks)
{
  sUBaseType_t timeoutFinish = SAT_ADD_U32(_sTickCount, timeoutTicks);
  __sCriticalRegionBegin();
  while (queueHandle->lenght == 0)
  {
    __sCriticalRegionEnd();
    if (timeoutFinish <= _sTickCount)
    {
      return srFalse;
    }
    sRTOSTaskYield();
    __sCriticalRegionBegin();
  }
  sUBaseType_t readPos = queueHandle->index % queueHandle->maxLenght;
  void *temp = queueHandle->items[readPos];

  memcpy(itemPtr, temp, queueHandle->itemSize);
  free(temp);
  queueHandle->lenght--;
  __sCriticalRegionEnd();
  return srTrue;
}

sbool_t sRTOSQueueSend(sQueueHandle_t *queueHandle, void *itemPtr, sUBaseType_t timeoutTicks)
{
  sUBaseType_t timeoutFinish = SAT_ADD_U32(_sTickCount, timeoutTicks);
  __sCriticalRegionBegin();
  while (queueHandle->lenght == queueHandle->maxLenght)
  {
    __sCriticalRegionEnd();
    if (timeoutFinish <= _sTickCount)
    {
      return srFalse;
    }
    sRTOSTaskYield();
    __sCriticalRegionBegin();
  }
  void *temp = malloc(queueHandle->itemSize);
  if (temp == NULL)
  {
    __sCriticalRegionEnd();
    return srFalse;
  }
  memcpy(temp, itemPtr, queueHandle->itemSize);
  queueHandle->index++;
  sUBaseType_t writePos = queueHandle->index % queueHandle->maxLenght;
  queueHandle->items[writePos] = temp;
  queueHandle->lenght++;
  __sCriticalRegionEnd();
  return srTrue;
}