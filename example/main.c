#include <stdint.h>
#include "stm32f4xx.h"

#include "simpleRTOS.h"

uint32_t count0, count1, count2, count3, count4;
uint32_t timercount0, timercount1, timercount2;

sTaskHandle_t Task0H;
sTaskHandle_t Task1H;
sTaskHandle_t Task2H;
sTimerHandle_t Timer0H;
sTimerHandle_t Timer1H;
sTimerHandle_t Timer2H;

void Timer0(sTimerHandle_t *h)
{
  timercount0++;
}
void Timer1(sTimerHandle_t *h)
{
  timercount1++;
}
void Timer2(sTimerHandle_t *h)
{
  timercount2++;
}

void Task0(void *)
{
  sRTOSTaskDelay(5);
  while (1)
  {
    count0++;
    if (count0 % 1000 == 0)
    {
      sRTOSTaskStop(&Task0H);
    }
  }
}

void Task1(void *)
{
  while (1)
  {
    count1++;
    if (count1 % 1000 == 0)
    {
      sRTOSTaskStop(&Task0H);
    }
  }
}

void Task2(void *)
{
  while (1)
  {
    count2++;
    if (count2 % 9000 == 0)
    {
      sRTOSTaskDelete(&Task0H);
    }
  }
}

int main(void)
{
  SystemCoreClockUpdate();
  sRTOSInit(SystemCoreClock);

  sRTOSTaskCreate(Task0,
                  "Task0",
                  NULL,
                  128,
                  sPriorityHigh,
                  &Task0H,
                  srFALSE);

  sRTOSTaskCreate(Task1,
                  "Task1",
                  NULL,
                  128,
                  sPriorityNormal,
                  &Task1H,
                  srTRUE);

  sRTOSTaskCreate(Task2,
                  "Task2",
                  NULL,
                  128,
                  sPriorityNormal,
                  &Task2H,
                  srTRUE);

  sRTOSTimerCreate(
      Timer0,
      1,
      2,
      srTrue,
      &Timer0H);

  sRTOSTimerCreate(
      Timer1,
      1,
      4,
      srTrue,
      &Timer1H);
  sRTOSTimerCreate(
      Timer2,
      1,
      5,
      srFalse,
      &Timer2H);

  sRTOSStartScheduler();
}
