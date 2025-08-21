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

#define __STATIC_FORCEINLINE__ __attribute__((always_inline)) static __inline
#define __STATIC_NAKED__ __attribute__((naked)) static

#define srFALSE 0u
#define srTRUE 1u

#define CONTEXT_STACK_SIZE 8
#define MIN_STACK_SIZE_NO_FPU 16
#define MIN_STACK_SIZE_FPU 49
#define MAX_TASK_NAME_LEN 12
#define MAX_TASK_PRIORITY_COUNT 32

#define SAT_ADD_U32(a, b) (((UINT32_MAX - (uint32_t)(a)) < (uint32_t)(b)) ? UINT32_MAX : (uint32_t)((uint32_t)(a) + (uint32_t)(b)))

typedef int32_t sBaseType_t;
typedef uint32_t sUBaseType_t;

typedef enum
{
  sFalse = 0u,
  sTrue
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
  sPriorityIdle = -16,
  sPriorityLow = -2,
  sPriorityBelowNormal = -1,
  sPriorityNormal = 0,
  sPriorityAboveNormal = 1,
  sPriorityHigh = 2,
  sPriorityRealtime = 15
};

#define sPriorityMin sPriorityIdle
#define sPriorityMax sPriorityRealtime

typedef signed char sPriority_t;

typedef enum
{
  sBlocked,
  sRunning,
  sReady,
  sDeleted,
  sWaiting
} sTaskStatus_t;

__attribute__((packed, aligned(4))) struct tcb
{
  sUBaseType_t *stackPt;
  struct tcb *nextTask;
  sbool_t fps; // floating-point state
  sTaskStatus_t status;
  sPriority_t priority;
  sbool_t regitersSaved; // tells the scheduler to save the registers if true
  sUBaseType_t *stackBase;
  struct tcb *prevTask;
  sUBaseType_t notificationMessage;
  sbool_t hasNotification;
  sPriority_t originalPriority; // this save the original priority of the task before being change by mutex
  char name[12];
};

typedef struct tcb sTaskHandle_t;

typedef struct __attribute__((packed, aligned(4)))
{
  sUBaseType_t *stackPt;    // Pointer to the stack
  sUBaseType_t *stackBase;  // Pointer to the Base of the stack
  sUBaseType_t id;          // Timer id
  sBaseType_t Period;       // Timer period in ticks (the period is relative to __sRTOS_SENSIBILITY)
  sbool_t autoReload;       // Timer autoReload
  sTaskStatus_t status;
} sTimerHandle_t;

typedef void (*sTaskFunc_t)(void *arg);
typedef void (*sTimerFunc_t)(sTimerHandle_t *timerHandle);
typedef sBaseType_t sSemaphore_t;
typedef struct
{
  sSemaphore_t sem;
  sTaskHandle_t *holderHandle;
  sTaskHandle_t *requesterHandle;
} sMutex_t;

typedef struct
{
  sUBaseType_t maxLenght;
  sUBaseType_t lenght;
  sUBaseType_t itemSize;
  sUBaseType_t index;
  void **items;
} sQueueHandle_t;

#endif /* SIMPLERTOSTYPES_H_ */
