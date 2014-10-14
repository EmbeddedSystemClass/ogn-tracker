#include "hpt_timer.h"
#include <stm32l1xx.h>
#include <FreeRTOS.h>
#include <FreeRTOS_CLI.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>
#include <timers.h>

//#define HPT_DEBUG 1

/* -------- defines -------- */
#define HPT_TIMER_ID  1
/* -------- variables -------- */
TimerHandle_t xHPTimer;
uint8_t       event_awaited;
HPT_Event*    current_hpt_table;

/* -------- interrupt handlers -------- */
/* -------- functions -------- */

#ifdef HPT_DEBUG
extern void Console_Send(const char* str, char block);
#endif
/**
* @brief  Restart the current HPT table.
* @param  None
* @retval None
*/
void HPT_Restart(void)
{
    if (current_hpt_table)
    {
       xTimerChangePeriod(xHPTimer, current_hpt_table[0].time, 0);
       /* mark information about event number that are awaited*/
       event_awaited = 0;
       /* start the timer */
       xTimerStart(xHPTimer, 0);
    }
}

/**
* @brief  High Precision Timer Callback.
* @param  Timer handle 
* @retval None
*/
void vHPTimerCallback(TimerHandle_t pxTimer)
{
    uint8_t new_event_scheduled = 0;
    switch (current_hpt_table[event_awaited].opcode)
    {
        case HPT_END:
            xTimerStop(xHPTimer, 0);
            new_event_scheduled = 1;
            break;
        case HPT_RESTART:
            #ifdef HPT_DEBUG
            Console_Send("HPT_RESTART\r\n",0);
            #endif
            HPT_Restart();
            new_event_scheduled = 1;
            break;
        case HPT_GPIO_UP:
            GPIO_SetBits(GPIOC, GPIO_Pin_5);
            #ifdef HPT_DEBUG
            Console_Send("HPT_GPIO_UP\r\n",0);
            #endif
            break;
        case HPT_GPIO_DOWN:
            GPIO_ResetBits(GPIOC, GPIO_Pin_5);
            #ifdef HPT_DEBUG
            Console_Send("HPT_GPIO_DOWN\r\n",0);
            #endif
            break;
        default:
            break;
    }
    /* In case of events not affecting hpt table timing next event from hpt table should be found */
    if (!new_event_scheduled)
    {
        /* find current time from table */
        uint32_t curr_time = current_hpt_table[event_awaited].time;
        /* get next event from table */
        event_awaited++;
        /* calculate amount of time to wait until next event */
        xTimerChangePeriod(xHPTimer, current_hpt_table[event_awaited].time - curr_time, 0);
        /* start the timer */
        xTimerStart(xHPTimer, 0);
        /* that's all, we'll see again in next vHPTimerCallback call */
    }
}

/**
* @brief  Starts the High Precision Timer.
* @param  HPT Events table
* @retval None
*/
void HPT_Start(HPT_Event* hpt_table)
{
   current_hpt_table = hpt_table;
   /* set timer expiration to first event in table */
   xTimerChangePeriod(xHPTimer, hpt_table[0].time, 0);
   /* mark information about event number that are awaited*/
   event_awaited = 0;
   /* start the timer */
   xTimerStart(xHPTimer, 0);
}

/**
* @brief  Configures the High Precision Timer.
* @param  None
* @retval None
*/
void HPT_Config(void)
{
   /* Test GPIO PC5 */
   GPIO_InitTypeDef GPIO_InitStructure;

   RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOC, ENABLE);

   GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
   GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
   GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
   GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
   GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_5;
   GPIO_Init(GPIOC, &GPIO_InitStructure);
   
   GPIO_ResetBits(GPIOC, GPIO_Pin_5);
   
   xHPTimer = xTimerCreate(
      "HPTimer",
      /* The timer period in ticks. */
      HPT_MS(1000),
      /* The timer will stop when expire. */
      pdFALSE,
      /* unique id */
      ( void * )HPT_TIMER_ID,
      /* Each timer calls the same callback when it expires. */
      vHPTimerCallback
    );
}