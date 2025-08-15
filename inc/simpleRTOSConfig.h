/*
 * simpleRTOSTypes.h
 *
 *  Created on: Aug 14, 2025
 *      Author: brachiGH
 */

#ifndef SIMPLERTOSCONFIG_H_
#define SIMPLERTOSCONFIG_H_

// #define __sUSE_PREEMPTION 1 // unavailable
#define __sQUANTA_MS 1  // rotate same priority tasks
#define __sTIMER_TASK_STACK_DEPTH 256 // in words
#define __sTIMER_LIST_LENGTH 12       // max number of timer

#endif