.syntax unified
.cpu cortex-m4
.thumb

.extern _sTickCount
.extern _sRTOS_TaskList
.extern _sRTOS_CurrentTask
.extern _sRTOSGetFirstAvailableTask
.extern _GetTimer
.extern _sIsTimerRunning
.extern _sQuantaCount
.extern __sQUANTA__
.global SysTick_Handler
.global sScheduler_Handler
.global sRTOSStartScheduler
.global sTimerReturn_Handler


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


.section .text.SysTick_Handler,"ax",%progbits
.type SysTick_Handler, %function
SysTick_Handler:
    cpsid   i                           // disable isr
    ldr     r0, =_sTickCount
    ldr     r1, [r0]
    adds    r1, #1
    str     r1, [r0]
    ldr     r0, =_sIsTimerRunning       //
    ldr     r0, [r0]                    // argument of _GetTimer
    cmp     r0, #1
    beq     2f
    push    {lr}
    bl      _GetTimer                   // get timer available else return null (this also decrement the timers)
    pop     {lr}
    cmp     r0, #0                      // check if NULL
    bne     1f                      
    // running scheduler_Handler if a quantom has passed
    ldr     r0, =_sQuantaCount
    ldr     r1, [r0]                    // read _sQuantaCount
    adds    r1, #1
    str     r1, [r0]                    // save _sQuantaCount=_sQuantaCount+1
    ldr     r3, =__sQUANTA__
    ldr     r3, [r3]                    // read __sQUANTA__
    cmp     r3, r1
    bne     2f                          
    // if _sQuantaCount==__sQUANTA__
    mov     r1, 0                       
    str     r1, [r0]                    // save _sQuantaCount=0
    b       sScheduler_Handler
1:
    b       sTimer_Handler
2:
    cpsie   i                           // enable isr  
    bx      lr                          // return
.size SysTick_Handler, .-SysTick_Handler




.section .text.sScheduler_Handler,"ax",%progbits
.type sScheduler_Handler, %function
sScheduler_Handler:                     // r0,r1,r2,r3,r12,lr,pc,psr   saved by interrupt
    ldr     r0, =_sRTOS_CurrentTask     // the _sRTOSSchedulerGetCurrent return the current task (r0 is the return value)
    ldr     r1, [r0]
    push    {lr}                        // save return address
    bl	    _sRTOSGetFirstAvailableTask // returns the first highest ready task
    pop     {lr}                        // restore return address
    ldr     r3, [r1, #4]                // r3 = CurrentTask->nextTask
    // r1 = currentTask, r0 = firstAvbleTask, r3 = nextTask
    ldrb    r2, [r0, #10]               // first available task priority
    ldrb    r12, [r3, #10]              // next task priority (next relative to the current task)
    cmp     r2, r12                     // cmp the nextTask and firstAvble priority
    // because the list task is order by priority 
    // if next and firstAvble tasks have the same priority
    // this means the next task should run an equal amout as current and firstAvble
    // note: that firstAvbleTask and nextTask could be the same Task
    beq    switchToRunningNextTask
    cmp     r1, r0                      // check if current and first avaible are the same
    bne    switchToRunningfirstAbleTask
    // if eq  then return execution to current task 
    cpsie   i                           // enable isr
    bx      lr                          // return the same task

switchToRunningfirstAbleTask:
    mov     r3, r0                      // changes the nextTask pointer to firstAvbleTask pointer
switchToRunningNextTask:
    ldrb    r0, [r1, #11]               // read current saveRegiters
    cmp     r0, #1                      // check if the task has to save regiters
    bne     2f
    mov     r0, #0
    strb	r0, [r1, #11]               // change current saveRegiters to false (to tell the scheduler that registers are saved)
    ldrb    r0, [r1, #9]                // read current status
    cmp     r0, #1                      // check if the task status is running (the status cloud change by a timer if so the register are saved)
    bne     1f
    mov     r0, #2                      //  sReady:0x02
    strb	r0, [r1, #9]                // change current task status to sReady
    push    {r4-r7}                     // save r4,r5,r6,r7,
    stmdb   sp!, {r8-r11}               //      r8,r9,r10,r11
    ldrb	r2, [r1, #8]                // CurrentTask->fps
    cmp     r2, #1                      // check if floating point stage is on
    bne     1f                          //
    vstmdb  sp!, {s0-s31}               // if float point mode is on save fpu registers of the current task
    vmrs    r0, fpscr                   //
    push    {r0}                        //
1:
    str     sp, [r1]                    // save the new current task sp
2:
    ldr     sp, [r3]                    // SP = nextTask->stackPt or firstAvbleTask->stackPt
    ldr	    r0, =0x1                    // sRunning:0x1
    strb	r0, [r3, #9]                // change next task status to sRunning

    ldr     r0, =_sRTOS_CurrentTask
    str     r3, [r0]                    // change the current running task ptr

    ldrb    r2, [r3, #8]                // nextTask->fps or firstAvbleTask->fps
    cmp     r2, #1                      //
    bne     3f                          //
    pop     {r0}                        // if float point mode is on restore fpu registers of the next task
    vmsr    fpscr, r0                   //
    vldmia  sp!, {s0-s31}               //
3:
    ldmia   sp!, {r8-r11}               //
    pop     {r4-r7}                     //
    mov     r0, #1
    strb	r0, [r3, #11]               // change current saveRegiters to true (tell the scheduler that it has to save the registers) 
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
    mov     r1, #1                      // change the _sIsTimerRunning to true
    str     r1, [r2]
    ldr     r2, =_sRTOS_CurrentTask     // the _sRTOSSchedulerGetCurrent return the current task (r0 is the return value)
    ldr     r1, [r2]
    ldrb    r3, [r1, #11]               // read current saveRegiters
    cmp     r3, #1                      // check if the task has to save regiters
    bne     3f
    mov     r3, #0
    strb	r3, [r1, #11]               // change current saveRegiters to false (to tell the scheduler that registers are saved)
    push    {r4-r7}                     // save r4,r5,r6,r7,
    stmdb   sp!, {r8-r11}               //      r8,r9,r10,r11
    ldrb	r2, [r1, #8]                // CurrentTask->fps
    cmp     r2, #1                      // check if floating point stage is on
    bne     2f                          //
    vstmdb  sp!, {s0-s31}               // if float point mode is on save fpu registers of the current task
    vmrs    r3, fpscr                   //
    push    {r3}                        //
2:
    str     sp, [r1]                    // save the new current task sp
3:
    ldr     sp, [r0]                    // SP = Timer Task
    cpsie   i                           // isr is enabled
    bx      lr                          // return and start the next task
.size sTimer_Handler, .-sTimer_Handler



.section .text.sTimerReturn_Handler,"ax",%progbits
.type sTimerReturn_Handler, %function
sTimerReturn_Handler:
    cpsid   i                           // disable isr
    ldr     r2, =_sRTOS_CurrentTask     // the _sRTOSSchedulerGetCurrent return the current task (r0 is the return value)
    ldr     r1, [r2]
    ldr     sp, [r1]                    // set task sp
    ldrb    r3, [r1, #11]               // read current saveRegiters
    cmp     r3, #0                      // check if the task has saved regiters
    bne     2f
    ldrb    r2, [r1, #8]                // nextTask->fps or firstAvbleTask->fps
    cmp     r2, #1                
    bne     1f                          //
    pop     {r0}                        // if float point mode is on restore fpu registers of the next task
    vmsr    fpscr, r0                   //
    vldmia  sp!, {s0-s31}               //
1:
    ldmia   sp!, {r8-r11}               //
    pop     {r4-r7}                     //
    mov     r0, #1
    strb	r0, [r3, #11]               // change current saveRegiters to true (tell the scheduler that it has to save the registers) 
2:
    cpsie   i                           // enable isr  
    bx      lr                          // return and start the next task
.size sTimerReturn_Handler, .-sTimerReturn_Handler


.section .text.sRTOSStartScheduler,"ax",%progbits
.type sRTOSStartScheduler, %function
sRTOSStartScheduler:
    cpsid   i                                   // disable isr
    ldr     r0, =_sRTOS_TaskList                //
    ldr     r2, [r0]                            //
    ldr     sp, [r2]                            //
    ldrb    r1, [r2, #8]                        // r2=_sRTOS_TaskList->fps
/*
    arm cmp r0 to true
    if true: add sp, sp, #166
    else: add sp, sp, #36
*/
    cmp     r1, #1
    ite     eq
    addeq   sp, sp, #166
    addne   sp, sp, #32
    pop     {r0}                                // pop PC
    add     sp, sp, #20
    pop     {r1}
    mov     lr, r1
    ldr	    r1, =0x1 
    strb	r1, [r2, #9]                      // change task status to sRunning
    ldr     r0, =_sRTOS_CurrentTask
    str     r2, [r0]                          // save the current task ptr
    // ...enable SysTick...
    ldr r0, =0xE000E010                        // #define SYST_CSR (*((volatile uint32_t *)0xE000E010))
    movs r1, #7                               // ENABLE | TICKINT | CLKSOURCE
    str r1, [r0]                              // enable SysTick, enable exception, select processor clock
    cpsie   i                                 // enable irq
    bx      lr
.size sRTOSStartScheduler, .-sRTOSStartScheduler
