/*
 * simpleRTOSHandlers.c
 *
 *  Created on: Aug 15, 2025
 *      Author: benhih
 */

#include "simpleRTOS.h"

sUBaseType_t volatile _sTickCount = 0;
sUBaseType_t volatile _sQuantaCount = 0;
sUBaseType_t volatile _sIsTimerRunning = 0;
sUBaseType_t __sQUANTA__ = __sQUANTA;

extern void sScheduler_Handler(void);
extern void sTimerReturn_Handler(void);

__attribute__((weak)) void SysTick_Handler(void) {}

__attribute__((naked)) void SVC_Handler(void)
{
  __sCriticalRegionBegin();

  // if the function is not naked, an other stack frame
  // is created for the SVC_Handler function.
  // And the address for this stack is moved into
  // MSP register, which leads to the wrong stack to be used

  __asm volatile(
      "TST lr, #4                   \n"
      "ITE EQ                       \n"
      "MRSEQ r0, MSP                \n"
      "MRSNE r0, PSP                \n"
      "mov     r1, r0               \n"
      "adds    r1, #24              \n" // uint8_t *pc = (uint8_t *)sp[6]; // stacked PC
      "ldr     r1, [r1, #0]         \n"
      "ldrb.w  r1, [r1, #-2]        \n" // uint8_t svc_number = pc[-2];
      "cmp     r1, #0               \n"
      "beq     sScheduler_Handler   \n"
      "cmp     r1, #1               \n"
      "beq     sTimerReturn_Handler \n"
      "cpsie   i                    \n"
      "bx      lr                   \n" ::: "memory");
}
