/*
*********************************************************************************************************
* Smart Parking - ipc.c
*********************************************************************************************************
*/

#include  "ipc.h"

/*
*********************************************************************************************************
* OBJECTS
*********************************************************************************************************
*/

OS_Q     ManagerQueue;
OS_Q     GateCommandQueue;
OS_Q     DisplayQueue;
OS_Q     LogQueue;
OS_TMR   GateTimer;

#define  IPC_MANAGER_Q_SIZE     16u
#define  IPC_GATE_Q_SIZE        8u
#define  IPC_DISPLAY_Q_SIZE     8u
#define  IPC_LOG_Q_SIZE         16u

#define  IPC_MSG_POOL_BLKS      16u
#define  IPC_DISP_POOL_BLKS     8u

/* ManagerQueue로 오가는 MANAGER_MSG 블록 풀 (OSQPost는 포인터만 전달하므로 실체는 풀에 둔다) */
static  OS_MEM      IpcMsgMem;
static  MANAGER_MSG IpcMsgPool[IPC_MSG_POOL_BLKS];

/* DisplayQueue로 오가는 DISPLAY_MSG 블록 풀 */
static  OS_MEM      IpcDispMem;
static  DISPLAY_MSG IpcDispPool[IPC_DISP_POOL_BLKS];

/*
*********************************************************************************************************
* LOCAL FUNCTIONS
*********************************************************************************************************
*/

/* 차단기 타이머 만료 콜백: 시간 트리거 이벤트도 큐 메시지로 변환한다. */
static void IPC_GateTimerCb(void *p_tmr, void *p_arg)
{
    MANAGER_MSG *p_msg;

    (void)p_tmr;
    (void)p_arg;

    p_msg = IPC_MsgAlloc();
    if (p_msg != (MANAGER_MSG *)0) {
        p_msg->type = MSG_GATE_TIMEOUT;
        IPC_PostManager(p_msg);
    }
}

/*
*********************************************************************************************************
* IPC_Init()
*********************************************************************************************************
*/

void IPC_Init(void)
{
    OS_ERR  err;

    OSQCreate(&ManagerQueue,     "Manager Queue",      IPC_MANAGER_Q_SIZE, &err);
    OSQCreate(&GateCommandQueue, "Gate Command Queue", IPC_GATE_Q_SIZE,    &err);
    OSQCreate(&DisplayQueue,     "Display Queue",      IPC_DISPLAY_Q_SIZE, &err);
    OSQCreate(&LogQueue,         "Log Queue",          IPC_LOG_Q_SIZE,     &err);

    OSMemCreate(&IpcMsgMem,
                "Manager Msg Pool",
                (void *)&IpcMsgPool[0],
                IPC_MSG_POOL_BLKS,
                sizeof(MANAGER_MSG),
                &err);

    OSMemCreate(&IpcDispMem,
                "Display Msg Pool",
                (void *)&IpcDispPool[0],
                IPC_DISP_POOL_BLKS,
                sizeof(DISPLAY_MSG),
                &err);

    /* dly 단위 = OS_CFG_TMR_TASK_RATE_HZ(10Hz) tick. 즉 N초 = N*10. */
    OSTmrCreate(&GateTimer,
                "Gate Auto-close Timer",
                (OS_TICK)(APP_GATE_OPEN_SEC * 10u),
                0u,                                     /* one-shot                                  */
                OS_OPT_TMR_ONE_SHOT,
                IPC_GateTimerCb,
                (void *)0,
                &err);
}

/*
*********************************************************************************************************
* MESSAGE HELPERS
*********************************************************************************************************
*/

MANAGER_MSG *IPC_MsgAlloc(void)
{
    OS_ERR       err;
    MANAGER_MSG *p_msg;

    p_msg = (MANAGER_MSG *)OSMemGet(&IpcMsgMem, &err);
    if (err != OS_ERR_NONE) {
        return (MANAGER_MSG *)0;
    }
    return p_msg;
}

void IPC_MsgFree(MANAGER_MSG *p_msg)
{
    OS_ERR  err;

    if (p_msg != (MANAGER_MSG *)0) {
        OSMemPut(&IpcMsgMem, (void *)p_msg, &err);
    }
}

void IPC_PostManager(MANAGER_MSG *p_msg)
{
    OS_ERR  err;

    OSQPost(&ManagerQueue,
            (void *)p_msg,
            sizeof(MANAGER_MSG),
            OS_OPT_POST_FIFO,
            &err);
}

DISPLAY_MSG *IPC_DisplayAlloc(void)
{
    OS_ERR       err;
    DISPLAY_MSG *p_msg;

    p_msg = (DISPLAY_MSG *)OSMemGet(&IpcDispMem, &err);
    if (err != OS_ERR_NONE) {
        return (DISPLAY_MSG *)0;
    }
    return p_msg;
}

void IPC_DisplayFree(DISPLAY_MSG *p_msg)
{
    OS_ERR  err;

    if (p_msg != (DISPLAY_MSG *)0) {
        OSMemPut(&IpcDispMem, (void *)p_msg, &err);
    }
}

void IPC_PostDisplay(DISPLAY_MSG *p_msg)
{
    OS_ERR  err;

    OSQPost(&DisplayQueue,
            (void *)p_msg,
            sizeof(DISPLAY_MSG),
            OS_OPT_POST_FIFO,
            &err);
}

void IPC_PostLog(LOG_EVENT event)
{
    OS_ERR  err;

    OSQPost(&LogQueue,
            (void *)event,                          /* enum 값을 포인터에 인코딩 */
            sizeof(void *),
            OS_OPT_POST_FIFO,
            &err);
}
