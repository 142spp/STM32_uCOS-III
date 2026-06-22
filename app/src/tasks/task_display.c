/*
*********************************************************************************************************
* Smart Parking - task_display.c
*
* DisplayQueue로 받은 상태를 1602 LCD와 2색 LED에 표시한다. LCD는 이 task만 접근한다.
*********************************************************************************************************
*/

#include  "task_display.h"
#include  "app_types.h"
#include  "ipc.h"
#include  "drv_lcd1602.h"

static  OS_TCB   DisplayTCB;
static  CPU_STK  DisplayStk[APP_TASK_STK_SIZE];

static void DisplayTask(void *p_arg)
{
    OS_ERR       err;
    OS_MSG_SIZE  size;
    void        *p_data;

    (void)p_arg;

    LCD_Init();
    LCD_Clear();

    while (DEF_TRUE) {
        p_data = OSQPend(&DisplayQueue, 0u, OS_OPT_PEND_BLOCKING, &size, (CPU_TS *)0, &err);
        if (err != OS_ERR_NONE) {
            continue;
        }
        (void)p_data;
        /* TODO: 받은 상태로 LCD 2줄 요약 출력 (예: "FREE:1/2 S1:O") 및 2색 LED 갱신 */
    }
}

void DisplayTask_Create(void)
{
    OS_ERR  err;

    OSTaskCreate(&DisplayTCB,
                 "Display Task",
                 DisplayTask,
                 (void *)0,
                 PRIO_DISPLAY,
                 &DisplayStk[0],
                 APP_TASK_STK_SIZE / 10u,
                 APP_TASK_STK_SIZE,
                 0u,
                 0u,
                 (void *)0,
                 OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR,
                 &err);
}
