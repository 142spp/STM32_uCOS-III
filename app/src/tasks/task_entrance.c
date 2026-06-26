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

#define  ENTRANCE_POLL_MS       50u
#define  ENTRANCE_COOLDOWN_MS   3000u           /* 입차 1회 후 이 시간 동안 추가 입차 무시 (센서 튐 방지) */
#define  ENTRANCE_COOLDOWN_CNT  (ENTRANCE_COOLDOWN_MS / ENTRANCE_POLL_MS)

static  OS_TCB   EntranceTCB;
static  CPU_STK  EntranceStk[APP_TASK_STK_SIZE];

static void EntranceTask(void *p_arg)
{
    OS_ERR       err;
    CPU_INT08U   prev = 0u;
    CPU_INT08U   now;
    CPU_INT16U   cooldown = 0u;                  /* 남은 불응 시간(폴링 tick). 0이어야 입차를 받는다. */
    MANAGER_MSG *p_msg;

    (void)p_arg;

    IR_Init();

    while (DEF_TRUE) {
        now = IR_IsDetected();

        if (cooldown > 0u) {
            cooldown--;                          /* 쿨다운 중: 차 한 대가 지나가며 튀는 동안은 무시 */
        } else if ((now != 0u) && (prev == 0u)) {
            /* 미감지 -> 감지로 바뀌는 순간(rising edge)에만 1회 요청 */
            p_msg = IPC_MsgAlloc();
            if (p_msg != (MANAGER_MSG *)0) {
                p_msg->type = MSG_ENTRANCE;
                IPC_PostManager(p_msg);
            }
            cooldown = ENTRANCE_COOLDOWN_CNT;     /* 다음 입차까지 불응 시간 시작 */
        }

        prev = now;
        OSTimeDlyHMSM(0u, 0u, 0u, ENTRANCE_POLL_MS, OS_OPT_TIME_HMSM_STRICT, &err);
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
