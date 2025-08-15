/*
 * simpleRTOS.c
 *
 *  Created on: Aug 14, 2025
 *      Author: brachiGH
 */

#include "simpleRTOSConfig.h"
#include "simpleRTOS.h"

sUBaseType_t __sRTOS_timerCount = 0;
sTimerHandle_t _sRTOS_timerLIST[__sTIMER_LIST_LENGTH];


sRTOS_StatusTypeDef sRTOSTimerCreate(
    sTimerFunc_t timerTask,
    sUBaseType_t id,
    sUBaseType_t period,
    sUBaseType_t autoReload,
    sTimerHandle_t *timerHandle,
    sUBaseType_t fpsMode)
{


    timerHandle->id =id;
    timerHandle->Period = period;
    timerHandle->autoReload = (sbool_t)autoReload;

    __sRTOS_timerCount++;
    return sRTOS_OK;
}
void sRTOSTimerDelete(sTimerHandle_t *timerHandle);
void sRTOSTimerUpdatePeriode(sTimerHandle_t *timerHandle);
