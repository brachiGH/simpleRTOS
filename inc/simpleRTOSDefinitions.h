/*
 * simpleRTOSTypes.h
 *
 *  Created on: Aug 14, 2025
 *      Author: brachiGH
 */

#ifndef SIMPLERTOSTYPES_H_
#define SIMPLERTOSTYPES_H_

#include <stddef.h>
#include <stdint.h>

#define __STATIC_FORCEINLINE __attribute__((always_inline)) static __inline 
#define __STATIC_NAKED __attribute__((naked)) static

#define srFALSE 0u
#define srTRUE 1u

#define MIN_STACK_SIZE_NO_FPU 16
#define MIN_STACK_SIZE_FPU 49
#define MAX_TASK_NAME_LEN 12

typedef int32_t sBaseType_t;
typedef uint32_t sUBaseType_t;

typedef enum
{
  srFalse = 0u,
  srTrue
} sbool_t;

typedef enum
{
  sRTOS_OK = 0x00U,
  sRTOS_ERROR = 0x80U,
  sRTOS_UNVALID_STACK_SIZE,
  sRTOS_UNVALID_PERIOD,
  sRTOS_ALLOCATION_FAILED,
  sRTOS_TIMER_LIST_IS_FULL,

} sRTOS_StatusTypeDef;

enum
{
  sPriorityIdle = -3,
  sPriorityLow = -2,
  sPriorityBelowNormal = -1,
  sPriorityNormal = 0,
  sPriorityAboveNormal = +1,
  sPriorityHigh = +2,
  sPriorityRealtime = +3
};

typedef signed char sPriority_t;

typedef enum
{
  sBlocked,
  sRunning,
  sReady,
  sWaiting,
  sDeleted
} sTaskStatus_t;

__attribute__((packed, aligned(4))) struct tcb
{
  sUBaseType_t *stackPt;
  struct tcb *nextTask;
  sbool_t fps; // floating-point state
  sTaskStatus_t status;
  sPriority_t priority;
  sbool_t saveRegiters;  // tells the scheduler to save the registers if true
  sUBaseType_t *stackBase;
  char name[12];
};

typedef struct tcb sTaskHandle_t;

typedef struct __attribute__((packed, aligned(4)))
{
  sUBaseType_t *stackPt;   // Pointer to the stack
  sUBaseType_t *stackBase; // Pointer to the Base of the stack
  sUBaseType_t id;         // Timer id
  sUBaseType_t Period;     // Timer period (the period is relative to __sRTOS_SENSIBILITY)
  sUBaseType_t ticksElapsed;
  sbool_t autoReload; // Timer autoReload
  sTaskStatus_t status;
} sTimerHandle_t;

typedef void (*sTaskFunc_t)(void *arg);
typedef void (*sTimerFunc_t)(sTimerHandle_t *timerHandle);
typedef sBaseType_t sSemaphore_t;

#endif /* SIMPLERTOSTYPES_H_ */
