/*
 * simpleRTOSConfig.h
 *
 *  Created on: Aug 14, 2025
 *      Author: brachiGH
 */

#ifndef SIMPLERTOSCONFIG_H_
#define SIMPLERTOSCONFIG_H_

/* RTOS sensibility is how responsive is the system*/
#define __sRTOS_SENSIBILITY_10MS 100 // above 1MS all function that use MS time as input will works multiples of __sRTOS_SENSIBILITY in this case multipules of 10MS
#define __sRTOS_SENSIBILITY_1MS 1000
#define __sRTOS_SENSIBILITY_500us 2000
#define __sRTOS_SENSIBILITY_250us 4000
#define __sRTOS_SENSIBILITY_100us 10000

#define __sRTOS_SENSIBILITY __sRTOS_SENSIBILITY_500us
/**************************************************/

// #define __sUSE_PREEMPTION 1        // unavailable
#define __sQUANTA 2                   // the quanta duration is relative to __sRTOS_SENSIBILITY
                                      // if sensibility is 100us then 1 quanta = 100us
                                      //(note:same priority tasks are rotate)
#define __sTIMER_TASK_STACK_DEPTH 256 // in words
#define __sTIMER_LIST_LENGTH 3        // Number of timers (keep the length of the list equal to the number of timer you create for best performance)
#define __sMAX_DELAY 0xFFFFFFFF

#endif
