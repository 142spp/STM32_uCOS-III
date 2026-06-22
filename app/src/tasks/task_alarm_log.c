/*
*********************************************************************************************************
* Smart Parking - task_alarm_log.c
*
* LogQueue로 받은 이벤트를 부저(경고)와 USART 로그로 출력한다.
* 모든 로그를 이 task로 라우팅하여 여러 task의 USART 출력이 뒤섞이는 것을 막는다.
*********************************************************************************************************
*/

#include  "task_alarm_log.h"
#include  "app_types.h"
#include  "ipc.h"
#include  "drv_buzzer.h"
#include  "drv_usart.h"

static  OS_TCB   AlarmLogTCB;
static  CPU_STK  AlarmLogStk[APP_TASK_STK_SIZE];

static void AlarmLogTask(void *p_arg)
{
    OS_ERR       err;
    OS_MSG_SIZE  size;
    void        *p_data;

    (void)p_arg;

    Buzzer_Init();

    while (DEF_TRUE) {
        p_data = OSQPend(&LogQueue, 0u, OS_OPT_PEND_BLOCKING, &size, (CPU_TS *)0, &err);
        if (err != OS_ERR_NONE) {
            continue;
        }
        /* LogQueue 메시지는 정적 문자열 포인터. USART 출력을 이 task로 단일화한다. */
        Usart_PutStr((const char *)p_data);

        /* TODO: 만차 등 경고 이벤트일 때 Buzzer_On()/Buzzer_Off() 처리 */
    }
}

void AlarmLogTask_Create(void)
{
    OS_ERR  err;

    OSTaskCreate(&AlarmLogTCB,
                 "Alarm Log Task",
                 AlarmLogTask,
                 (void *)0,
                 PRIO_ALARM_LOG,
                 &AlarmLogStk[0],
                 APP_TASK_STK_SIZE / 10u,
                 APP_TASK_STK_SIZE,
                 0u,
                 0u,
                 (void *)0,
                 OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR,
                 &err);
}
