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
    LOG_EVENT    event;

    (void)p_arg;

    Buzzer_Init();

    while (DEF_TRUE) {
        p_data = OSQPend(&LogQueue, 0u, OS_OPT_PEND_BLOCKING, &size, (CPU_TS *)0, &err);
        if (err != OS_ERR_NONE) {
            continue;
        }
        event = (LOG_EVENT)(CPU_ADDR)p_data;        /* 포인터에 인코딩된 이벤트 코드 */

        /* 로그 문자열과 부저는 이 task가 결정 (USART 출력 단일화) */
        switch (event) {
            case LOG_ENTRANCE_DETECTED:
                Usart_PutStr("[EVENT] entrance_detected\r\n");
                break;

            case LOG_GATE_OPEN:
                Usart_PutStr("[GATE] open\r\n");
                break;

            case LOG_GATE_CLOSE:
                Usart_PutStr("[GATE] close\r\n");
                break;

            case LOG_FULL_DENIED:
                Usart_PutStr("[ALARM] parking_full entrance_denied\r\n");
                Buzzer_On();
                OSTimeDlyHMSM(0u, 0u, 0u, 500u, OS_OPT_TIME_HMSM_STRICT, &err);
                Buzzer_Off();
                break;

            case LOG_EXIT:
                Usart_PutStr("[BUTTON] exit_event_logged\r\n");
                break;

            case LOG_RESERVE_EXPIRED:
                Usart_PutStr("[EVENT] reservation_expired\r\n");
                break;

            default:
                break;
        }
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
