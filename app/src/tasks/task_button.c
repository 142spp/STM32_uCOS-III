/*
*********************************************************************************************************
* Smart Parking - task_button.c
*
* 출차 버튼 ISR이 세마포어만 post하고, 이 task가 디바운스 후 ManagerQueue(MSG_EXIT)로 전달한다.
* (기존 app.c의 버튼 ISR -> 세마포어 -> 큐 패턴을 재사용)
*********************************************************************************************************
*/

#include  "task_button.h"
#include  "app_types.h"
#include  "ipc.h"

#define  BUTTON_DEBOUNCE_MS   50u

static  OS_TCB   ButtonTCB;
static  CPU_STK  ButtonStk[APP_TASK_STK_SIZE];
static  OS_SEM   ButtonSem;

void ButtonTask_SignalFromISR(void)
{
    OS_ERR  err;
    OSSemPost(&ButtonSem, OS_OPT_POST_1, &err);
}

static void ButtonTask(void *p_arg)
{
    OS_ERR       err;
    MANAGER_MSG *p_msg;

    (void)p_arg;

    OSSemCreate(&ButtonSem, "Button Semaphore", 0u, &err);

    while (DEF_TRUE) {
        OSSemPend(&ButtonSem, 0u, OS_OPT_PEND_BLOCKING, (CPU_TS *)0, &err);
        OSTimeDlyHMSM(0u, 0u, 0u, BUTTON_DEBOUNCE_MS, OS_OPT_TIME_HMSM_STRICT, &err);

        /* TODO: 디바운스 후 버튼이 여전히 눌려 있는지 확인 */
        p_msg = IPC_MsgAlloc();
        if (p_msg != (MANAGER_MSG *)0) {
            p_msg->type = MSG_EXIT;
            IPC_PostManager(p_msg);
        }
    }
}

void ButtonTask_Create(void)
{
    OS_ERR  err;

    OSTaskCreate(&ButtonTCB,
                 "Button Task",
                 ButtonTask,
                 (void *)0,
                 PRIO_BUTTON,
                 &ButtonStk[0],
                 APP_TASK_STK_SIZE / 10u,
                 APP_TASK_STK_SIZE,
                 0u,
                 0u,
                 (void *)0,
                 OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR,
                 &err);
}
