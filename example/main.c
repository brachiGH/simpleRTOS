#include <stdint.h>
#include "stm32f4xx.h"
#include "simpleRTOS.h"

uint32_t count0, count1, count2, count3, count4;
uint32_t timercount0, timercount1, timercount2;

sTaskHandle_t Task0H;
sTaskHandle_t Task1H;
sTaskHandle_t Task2H;
sTaskHandle_t Task3H;
sTaskHandle_t Task4H;
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
  while (1)
  {
    count0++;
    if (count0 == 100000)
    {
      sRTOSTaskStop(&Task4H);
    }
  }
}

void Task1(void *)
{
  while (1)
  {
    count1++;
  }
}

void Task2(void *)
{
  while (1)
  {
    count2++;
  }
}

void Task3(void *)
{
  sRTOSTaskDelay(5);
  while (1)
  {
    count3++;
    if ((count3 % 1000) == 0)
    {
      sRTOSTaskDelay(5);
    }
  }
}

void Task4(void *)
{
  sRTOSTaskDelay(10);
  while (1)
  {
    count4++;
    if ((count4 % 1000) == 0)
    {
      sRTOSTaskDelay(5);
    }
  }
}

int main(void)
{
  SystemCoreClockUpdate();
  sRTOSInit(SystemCoreClock);

  sRTOSTaskCreate(Task4,
                  "Task4",
                  NULL,
                  128,
                  sPriorityHigh,
                  &Task4H,
                  sFalse);
  sRTOSTaskCreate(Task3,
                  "Task3",
                  NULL,
                  128,
                  sPriorityHigh,
                  &Task3H,
                  sFalse);
  sRTOSTaskCreate(Task2,
                  "Task2",
                  NULL,
                  128,
                  sPriorityNormal,
                  &Task2H,
                  sTrue);

  sRTOSTaskCreate(Task1,
                  "Task1",
                  NULL,
                  128,
                  sPriorityNormal,
                  &Task1H,
                  sTrue);
  sRTOSTaskCreate(Task0,
                  "Task0",
                  NULL,
                  128,
                  sPriorityNormal,
                  &Task0H,
                  sFalse);

  sRTOSTimerCreate(
      Timer0,
      1,
      80,
      sTrue,
      &Timer0H);

  sRTOSTimerCreate(
      Timer1,
      1,
      160,
      sTrue,
      &Timer1H);
  sRTOSTimerCreate(
      Timer2,
      1,
      5,
      sFalse,
      &Timer2H);

  sRTOSStartScheduler();
}
