.syntax unified
.cpu cortex-m4
.thumb

.extern _sTickCount
.extern _sCurrentTask
.extern _sRTOSGetFirstAvailableTask
.extern _sCheckDelays
.extern _sIsTimerRunning
.extern _sTicksPassedExecutingCurrentTask
.global SysTick_Handler
.global SVC_Handler
.global sScheduler_Handler
.global sRTOSStartScheduler
.global sTimerReturn_Handler

#include "simpleRTOSConfig.h"



.section .text.SysTick_Handler,"ax",%progbits
.type SysTick_Handler, %function
SysTick_Handler:
    cpsid   i                           // disable isr
    ldr     r0, =_sTickCount
    ldr     r1, [r0]                    // read _sTicksPassedExecutingCurrentTask
    adds    r1, #1
    str     r1, [r0]                    // save _sTickCount++

    ldr     r0, =_sTicksPassedExecutingCurrentTask
    ldr     r1, [r0]                    // read _sTicksPassedExecutingCurrentTask
    adds    r1, #1
    str     r1, [r0]                    // save _sTicksPassedExecutingCurrentTask++

    ldr     r0, =_sIsTimerRunning       //
    ldr     r0, [r0]                    // read value of _sIsTimerRunning
    cmp     r0, #1
    beq     2f                          // if a timer if running 
    push    {lr}
    bl      _sCheckDelays               // get timer available else return null (this also decrement the timers)
    pop     {lr}
    cmp     r0, #0                      // check if NULL
    bne     sTimer_Handler              // running scheduler_Handler if a quantom has passed
    b       sScheduler_Handler
.size SysTick_Handler, .-SysTick_Handler



.section .text.SVC_Handler,"ax",%progbits
.type SVC_Handler, %function
SVC_Handler:
    cpsid   i                           // disable isr
    TST lr, #4                   
    ITE EQ                       
    MRSEQ r0, MSP                
    MRSNE r0, PSP                
    mov     r1, r0               
    adds    r1, #24                     // uint8_t *pc = (uint8_t *)sp[6]; // stacked PC
    ldr     r1, [r1, #0]         
    ldrb.w  r1, [r1, #-2]               // uint8_t svc_number = pc[-2];
    cmp     r1, #0                      // yeid
    beq     sScheduler_Handler_andRest   
    cmp     r1, #1               
    beq     sTimerReturn_Handler        // timer Return
    cpsie   i                    
    bx      lr                  

sScheduler_Handler_andRest:
    ldr     r1, =_sTicksPassedExecutingCurrentTask
    mov	    r2, #__sQUANTA
    str     r2, [r1]                    // rest _sTicksPassedExecutingCurrentTask
    b       sScheduler_Handler
.size SVC_Handler, .-SVC_Handler



/*
    
When an interrupt occurs on ARM Cortex-M:

1. **Hardware automatically saves context:**  
   The processor pushes these registers onto the current stack (MSP or PSP):
   - `r0`, `r1`, `r2`, `r3`, `r12`, `lr`, `pc`, `xPSR`

2. **Software can save more context:**  
   If needed, the interrupt handler (or RTOS) can manually push additional 
   registers (`r4`–`r11`, etc.) to the stack.

3. **On return:**  
   The processor pops the saved registers from the stack, restoring the previous 
   state and resuming execution.

**Summary:**  
Hardware saves core registers automatically; software can save more if needed.
This ensures the interrupted code resumes correctly after the interrupt.
    
*/

.section .text.sTaskSaveRegisters,"ax",%progbits
.type sTaskSaveRegisters, %function      
sTaskSaveRegisters:
// argument r0: is TaskHandle for the task you want to save it registers
// note: r1 is changed in this routine
// use register to save the lr before jumping to this routine because the satck will change
    ldrb    r1, [r0, #11]               // read current regitersSaved
    cmp     r1, #1                      // check if the task has saved regiters
    beq     1f
    push    {r4-r7}                     // save r4,r5,r6,r7,
    stmdb   sp!, {r8-r11}               //      r8,r9,r10,r11
    ldrb	r1, [r0, #8]                // Task->fps
    cmp     r1, #1                      // check if floating point stage is on
    bne     1f                          //
    vstmdb  sp!, {s0-s31}               // if float point mode is on save fpu registers of the current task
    vmrs    r1, fpscr                   //
    push    {r1}                        //
1:
    mov     r1, #1
    strb	r1, [r0, #11]               // change RegitersSaved to true
    str     sp, [r0]                    // save the new current task sp
    bx      lr
.size sTaskSaveRegisters, .-sTaskSaveRegisters

.section .text.sTaskRestoreRegisters,"ax",%progbits
.type sTaskRestoreRegisters, %function
sTaskRestoreRegisters:
// argument r0: is TaskHandle for the task you want to save it registers
// note: r1 is changed in this routine
// use register to save the lr before jumping to this routine because the satck will change
    ldr     sp, [r0]                    // set task sp
    ldrb    r1, [r0, #11]               // read current regitersSaved
    cmp     r1, #1                      // check if the task has saved regiters
    bne     1f
    ldrb    r1, [r0, #8]                // Task->fps
    cmp     r1, #1                      //
    bne     1f                          //
    pop     {r1}                        // if float point mode is on restore fpu registers of the next task
    vmsr    fpscr, r1                   //
    vldmia  sp!, {s0-s31}               //
1:
    ldmia   sp!, {r8-r11}               //
    pop     {r4-r7}                     //
    mov     r1, #0
    strb	r1, [r0, #11]               // change RegitersSaved to false
    bx      lr
.size sTaskRestoreRegisters, .-sTaskRestoreRegisters


.section .text.sScheduler_Handler,"ax",%progbits
.type sScheduler_Handler, %function
sScheduler_Handler:                     // r0,r1,r2,r3,r12,lr,pc,psr   saved by interrupt
    push    {lr}                        // save return address
    bl	    _sRTOSGetFirstAvailableTask // returns the first highest ready task
    pop     {lr}                        // restore return address
    cmp     r0, #0                      // if AvailableTask is null then keep executing current task
    beq     2f                          // if eq to NULL then return execution to current task
    mov     r2, r0
    ldr     r0, =_sCurrentTask          // the _sCurrentTask return the current task (r0 is the return value)
    ldr     r0, [r0]                    // load the addres of current task
    mov     r3, lr
    bl      sTaskSaveRegisters
    mov     lr, r3
    ldrb    r3, [r0, #9]                // read current status
    cmp     r3, #1                      // check if the task status is running (the status cloud change by a timer if so the register are saved)
    bne     1f
    mov     r3, #2                      // sReady:0x02
    strb	r3, [r0, #9]                // change current task status to sReady
1:
    mov     r0, r2                      // copy firstAvailableTask addr into r0
    mov     r3, lr
    bl      sTaskRestoreRegisters
    mov     lr, r3
    ldr	    r1, =0x1                    // sRunning:0x1
    strb	r1, [r0, #9]                // change next task status to sRunning

    ldr     r1, =_sCurrentTask
    str     r0, [r1]                    // change the current running task ptr
2:
    cpsie   i                           // enable isr  
    bx      lr                          // return and start the next task
.size sScheduler_Handler, .-sScheduler_Handler

/*
When returning from an interrupt on ARM Cortex-M, **context restore**
means putting back all the CPU registers and state so the interrupted code can resume
as if nothing happened.

### How Context Restore Works

1. **Hardware context restore:**  
   - When an interrupt occurs, the hardware automatically pushes
   `r0`, `r1`, `r2`, `r3`, `r12`, `lr`, `pc`, and `xPSR` onto the stack.
   - When returning, the hardware pops these registers off the stack.

2. **Software context restore:**  
   - If the interrupt handler or RTOS saved more registers (like `r4`–`r11`),
   it must restore them before returning.

3. **Returning with `bx lr` and `lr = 0xFFFFFFF9`:**  
   - In ARM Cortex-M, the **link register (`lr`)** is set to a special value called
   **EXC_RETURN** (e.g., `0xFFFFFFF9`) during an exception.
   - Executing `bx lr` with `lr = 0xFFFFFFF9` tells the processor:
     - "I am done with the interrupt. Restore the context from the
     stack and resume execution in Thread mode using the Main Stack Pointer (MSP)."
   - The processor automatically pops the saved hardware registers and resumes the interrupted code.

---

**Summary:**  
- `bx lr` with `lr = 0xFFFFFFF9` triggers the ARM Cortex-M exception return mechanism.
- The processor restores all hardware-saved registers and resumes execution where the
    interrupt occurred.

*/


.section .text.sTimer_Handler,"ax",%progbits
.type sTimer_Handler, %function
sTimer_Handler:                         // r0,r1,r2,r3,r12,lr,pc,psr   saved by interrupt
    ldr     r2, =_sIsTimerRunning
    mov     r1, #1                     
    str     r1, [r2]                    // change the _sIsTimerRunning to true
    mov     r2, r0                      // save a copy of r0 which is the the timerHandle into r1
    ldr     r0, =_sCurrentTask          // the _sCurrentTask return the current task (r0 is the return value)
    ldr     r0, [r0]

    mov     r3, lr
    bl      sTaskSaveRegisters
    mov     lr, r3

    mov     r0, r2                      // restore the the timerHandle to r0, because it the argument to the for the timer
    ldr     sp, [r0]                    // SP = Timer Task
    cpsie   i                           // isr is enabled
    bx      lr                          // return and start the next task
.size sTimer_Handler, .-sTimer_Handler



.section .text.sTimerReturn_Handler,"ax",%progbits
.type sTimerReturn_Handler, %function
sTimerReturn_Handler:
    cpsid   i                           // disable isr
    ldr     r0, =_sCurrentTask          // the _sCurrentTask return the current task (r0 is the return value)
    ldr     r0, [r0]
    mov     r3, lr
    bl      sTaskRestoreRegisters
    mov     lr, r3
    b       sScheduler_Handler         // rerun the scheduler in case the timer run at the end of quantum (which mean that the next task should run)
.size sTimerReturn_Handler, .-sTimerReturn_Handler


.section .text.sRTOSStartScheduler,"ax",%progbits
.type sRTOSStartScheduler, %function
sRTOSStartScheduler:
    cpsid   i                                   // disable isr
    push    {lr}                                // save return address
    bl	    _sRTOSGetFirstAvailableTask         // returns the first highest ready task
    pop     {lr}                                // restore return address
    ldr     sp, [r0]                            // switch to to the sp of the task
    ldrb    r1, [r0, #8]                        // r2=task->fps
/*
    arm cmp r0 to true
    if true: add sp, sp, #166
    else: add sp, sp, #36
*/
    cmp     r1, #1                              // check if task->fps is on
    ite     eq
    addeq   sp, sp, #166                      // #166 represent the number of bytes used to restore the register if floating point mode is on
    addne   sp, sp, #32                         // #31 epresent the number of bytes used to restore the register if floating point mode is off
    add     sp, sp, #24 
    pop     {r1}                                // pop the address of the task to execute
    mov     lr, r1                              // setting the return address to the address of the task so when `sRTOSStartScheduler` finishs it return to the address of the task instad of `main`
    ldr	    r1, =0x1                            // sRunning:0x1
    strb	r1, [r0, #9]                        // change task status to sRunning
    ldr     r2, =_sCurrentTask                  // get the address of _sCurrentTask
    str     r0, [r2]                            // save the current task ptr
    mov     r1, #0
    strb	r1, [r0, #11]                       // change regitersSaved to false (because registers are restored)
    // ...enable SysTick...
    ldr r0, =0xE000E010                         // #define SYST_CSR (*((volatile uint32_t *)0xE000E010))
    movs r1, #7                                 // ENABLE | TICKINT | CLKSOURCE
    str r1, [r0]                                // enable SysTick, enable exception, select processor clock
    cpsie   i                                   // enable irq
    bx      lr                                  // return
.size sRTOSStartScheduler, .-sRTOSStartScheduler
