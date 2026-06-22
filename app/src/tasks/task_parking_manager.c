/*
*********************************************************************************************************
* Smart Parking - task_parking_manager.c
*
* 단일 디스패처(single dispatcher). ManagerQueue로 들어오는 모든 메시지를 순서대로 처리한다.
* 주차칸 상태/빈자리 수/차단기 상태는 이 파일 내부에만 존재하며, 다른 task는 큐로만 요청한다.
*********************************************************************************************************
*/

#include  "task_parking_manager.h"
#include  "app_types.h"
#include  "ipc.h"

static  OS_TCB   ParkingManagerTCB;
static  CPU_STK  ParkingManagerStk[APP_TASK_STK_SIZE];

/* ---- ParkingManager만 소유하는 사적 상태 (락 불필요) ---- */
static  SLOT_STATE  SlotState[APP_SLOT_COUNT];
static  CPU_INT08U  FreeCount;
static  CPU_BOOLEAN GateOpen;

/*
*********************************************************************************************************
* LOCAL HELPERS (TODO: 실제 로직 채우기)
*********************************************************************************************************
*/

static void Manager_SendGate(GATE_CMD cmd)
{
    OS_ERR  err;
    OSQPost(&GateCommandQueue, (void *)cmd, sizeof(GATE_CMD), OS_OPT_POST_FIFO, &err);
}

static void Manager_RecountFree(void)
{
    CPU_INT08U i;
    FreeCount = 0u;
    for (i = 0u; i < APP_SLOT_COUNT; i++) {
        if (SlotState[i] == SLOT_EMPTY) {
            FreeCount++;
        }
    }
}

/* 측정 거리로 한 칸의 점유 상태 갱신. RESERVED는 점유 확정 시에만 OCCUPIED로 바뀐다. */
static void Manager_OnSensor(CPU_INT08U slot, CPU_INT16U dist_mm)
{
    CPU_BOOLEAN occupied;

    if (slot >= APP_SLOT_COUNT) {
        return;
    }
    occupied = (dist_mm < APP_OCCUPIED_THRESHOLD_MM) ? DEF_TRUE : DEF_FALSE;

    /* TODO: 히스테리시스/연속 N회 일치로 떨림 방지 */
    if (occupied) {
        SlotState[slot] = SLOT_OCCUPIED;        /* RESERVED -> OCCUPIED 확정 포함 */
    } else if (SlotState[slot] != SLOT_RESERVED) {
        SlotState[slot] = SLOT_EMPTY;           /* 예약 중인 칸은 비우지 않는다 */
    }
    Manager_RecountFree();
}

/* 입차 요청: 빈칸이 있으면 예약 후 차단기 개방, 없으면 거부. */
static void Manager_OnEntrance(void)
{
    CPU_INT08U i;

    if (FreeCount > 0u) {
        for (i = 0u; i < APP_SLOT_COUNT; i++) {
            if (SlotState[i] == SLOT_EMPTY) {
                SlotState[i] = SLOT_RESERVED;   /* 마지막 한 자리 중복 허용 방지 */
                break;
            }
        }
        Manager_RecountFree();
        Manager_SendGate(GATE_OPEN);
        GateOpen = DEF_TRUE;
        /* TODO: OSTmrStart(&GateTimer) 로 N초 자동 닫힘 시작 */
    } else {
        Manager_SendGate(GATE_DENY);            /* 만차: LED/부저/로그는 Gate/Alarm task가 처리 */
    }
}

static void Manager_OnGateTimeout(void)
{
    Manager_SendGate(GATE_CLOSE);
    GateOpen = DEF_FALSE;
}

static void Manager_OnExit(void)
{
    /* 버튼은 상태를 직접 비우지 않는다. 출차 이벤트만 기록. 빈자리는 초음파로만 갱신. */
    /* TODO: LogQueue로 출차 이벤트 전송 */
}

/*
*********************************************************************************************************
* TASK
*********************************************************************************************************
*/

static void ParkingManagerTask(void *p_arg)
{
    OS_ERR        err;
    OS_MSG_SIZE   size;
    MANAGER_MSG  *p_msg;
    CPU_INT08U    i;

    (void)p_arg;

    for (i = 0u; i < APP_SLOT_COUNT; i++) {
        SlotState[i] = SLOT_EMPTY;
    }
    FreeCount = APP_SLOT_COUNT;
    GateOpen  = DEF_FALSE;

    while (DEF_TRUE) {
        /* 이벤트가 없으면 여기서 잠들어 CPU를 양보, 메시지가 오면 즉시 깨어난다. */
        p_msg = (MANAGER_MSG *)OSQPend(&ManagerQueue, 0u, OS_OPT_PEND_BLOCKING, &size, (CPU_TS *)0, &err);
        if (err != OS_ERR_NONE || p_msg == (MANAGER_MSG *)0) {
            continue;
        }

        switch (p_msg->type) {
            case MSG_SENSOR:        Manager_OnSensor(p_msg->slot_id, p_msg->distance_mm); break;
            case MSG_ENTRANCE:      Manager_OnEntrance();                                 break;
            case MSG_EXIT:          Manager_OnExit();                                     break;
            case MSG_GATE_TIMEOUT:  Manager_OnGateTimeout();                              break;
            default:                                                                      break;
        }

        IPC_MsgFree(p_msg);

        /* TODO: DisplayQueue로 현재 상태(빈자리 수/칸 상태/차단기) 전송 */
    }
}

void ParkingManagerTask_Create(void)
{
    OS_ERR  err;

    OSTaskCreate(&ParkingManagerTCB,
                 "Parking Manager Task",
                 ParkingManagerTask,
                 (void *)0,
                 PRIO_PARKING_MANAGER,
                 &ParkingManagerStk[0],
                 APP_TASK_STK_SIZE / 10u,
                 APP_TASK_STK_SIZE,
                 0u,
                 0u,
                 (void *)0,
                 OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR,
                 &err);
}
