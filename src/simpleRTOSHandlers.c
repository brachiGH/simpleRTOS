/*
 * simpleRTOSHandlers.c
 *
 *  Created on: Aug 15, 2025
 *      Author: benhih
 */

#include "simpleRTOS.h"
#include "simpleRTOSConfig.h"

sUBaseType_t _sTickCount = 0;
sUBaseType_t _sIsTimerRunning = 0;

extern void sScheduler_Handler(void);
extern void sTimer_Handler(void);

// this fucntion must not use any register other then r0,r1,r2,r3,r12
// even after compiling it.
__attribute__((naked)) void SysTick_Handler(void)
{
  _sTickCount++;

  __asm volatile(
      "b sTimer_Handler" ::: "memory");

  if (_sTickCount % __sQUANTA == 0 && !_sIsTimerRunning)
    __asm volatile(
        "b sScheduler_Handler\n" ::: "memory");

}

__attribute__((naked)) void SVC_Handler(void)
{
  // if the function is not naked, an other stack frame
  // is created for the SVC_Handler function.
  // And the address for this stack is moved into
  // MSP register, which leads to the wrong stack to be
  // passed to the SVC_Handler_c
  // The register value of r0 is the first argument in
  // SVC_Handler_c function, which called by 'B SVC_Handler_c'

  __asm volatile(
      "TST lr, #4        \n"
      "ITE EQ            \n"
      "MRSEQ r0, MSP     \n"
      "MRSNE r0, PSP     \n"
      "B SVC_Handler_c   \n");
}

__attribute__((naked)) void SVC_Handler_c(uint32_t *sp)
{
  // to better inderstand the sp value refere to the image below
  // the sp points to the top of the stack which mean the r0 register
  // of the svc_add fucntion (aka it fisrt argument 'a')
  __asm volatile(
      "mov     r1, r0               \n"
      "adds    r1, #24              \n" // uint8_t *pc = (uint8_t *)sp[6]; // stacked PC
      "ldr     r1, [r1, #0]         \n"
      "ldrb.w  r1, [r1, #-2]        \n" // uint8_t svc_number = pc[-2];
      "cmp     r1, #0               \n"
      "beq     sScheduler_Handler   \n"
      "bx      lr                   \n" ::: "memory");
  // why the pc value contain the svc number?
  // The PC (Program Counter) is a CPU register that holds the
  // address of the next instruction to execute
  // and this address points to the epilogue of the
  // svc_add function (image below) which is a NOP in our case
}
