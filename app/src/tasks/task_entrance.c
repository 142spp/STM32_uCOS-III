/*
*********************************************************************************************************
* Smart Parking - task_entrance.c
*
* 적외선 센서로 입구 차량 접근을 감지하여 ManagerQueue(MSG_ENTRANCE)로 입차 요청을 보낸다.
*********************************************************************************************************
*/

#include  "task_entrance.h"
#include  "app_types.h"
#include  "ipc.h"
#include  "drv_ir.h"

static  OS_TCB   EntranceTCB;
static  CPU_STK  EntranceStk[APP_TASK_STK_SIZE];

static void EntranceTask(void *p_arg)
{
    OS_ERR       err;
    CPU_INT08U   prev = 0u;
    CPU_INT08U   now;
    MANAGER_MSG *p_msg;

    (void)p_arg;

    IR_Init();

    while (DEF_TRUE) {
        now = IR_IsDetected();
        /* 미감지 -> 감지로 바뀌는 순간(rising edge)에만 1회 요청 */
        if ((now != 0u) && (prev == 0u)) {
            p_msg = IPC_MsgAlloc();
            if (p_msg != (MANAGER_MSG *)0) {
                p_msg->type = MSG_ENTRANCE;
                IPC_PostManager(p_msg);
            }
        }
        prev = now;
        OSTimeDlyHMSM(0u, 0u, 0u, 50u, OS_OPT_TIME_HMSM_STRICT, &err);       /* 폴링 주기 (TODO)      */
    }
}

void EntranceTask_Create(void)
{
    OS_ERR  err;

    OSTaskCreate(&EntranceTCB,
                 "Entrance Task",
                 EntranceTask,
                 (void *)0,
                 PRIO_ENTRANCE,
                 &EntranceStk[0],
                 APP_TASK_STK_SIZE / 10u,
                 APP_TASK_STK_SIZE,
                 0u,
                 0u,
                 (void *)0,
                 OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR,
                 &err);
}
