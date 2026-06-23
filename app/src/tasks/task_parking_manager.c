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

/* 히스테리시스: 같은 판정이 몇 회 연속됐는지 센다. 둘 중 하나만 누적된다. */
static  CPU_INT08U  OccCount[APP_SLOT_COUNT];   /* 연속 "점유" 판정 횟수 */
static  CPU_INT08U  EmpCount[APP_SLOT_COUNT];   /* 연속 "비점유" 판정 횟수 */

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

/* 현재 상태 스냅샷을 DisplayTask로 전송 */
static void Manager_SendDisplay(void)
{
    DISPLAY_MSG *p_disp;
    CPU_INT08U   i;

    p_disp = IPC_DisplayAlloc();
    if (p_disp == (DISPLAY_MSG *)0) {
        return;
    }
    p_disp->free_count = FreeCount;
    p_disp->gate_open  = GateOpen;
    for (i = 0u; i < APP_SLOT_COUNT; i++) {
        p_disp->slot[i] = SlotState[i];
    }
    IPC_PostDisplay(p_disp);
}

/* 측정 거리로 한 칸의 점유 상태 갱신.
 * - 측정 실패(INVALID)는 무시한다 (떨림으로 카운트하지 않음).
 * - 같은 판정이 APP_SENSOR_HYST_CNT회 연속될 때만 상태를 바꾼다 (히스테리시스).
 * - RESERVED는 점유 확정(차가 실제로 들어옴) 시에만 OCCUPIED로 바뀐다.
 */
static void Manager_OnSensor(CPU_INT08U slot, CPU_INT16U dist_mm)
{
    CPU_BOOLEAN occupied;

    if (slot >= APP_SLOT_COUNT) {
        return;
    }
    if (dist_mm == APP_DISTANCE_INVALID) {
        return;                                 /* 측정 실패는 어느 쪽으로도 카운트하지 않는다 */
    }

    occupied = (dist_mm < APP_OCCUPIED_THRESHOLD_MM) ? DEF_TRUE : DEF_FALSE;

    if (occupied) {
        EmpCount[slot] = 0u;
        if (OccCount[slot] < APP_SENSOR_HYST_CNT) {
            OccCount[slot]++;
        }
        if (OccCount[slot] >= APP_SENSOR_HYST_CNT) {
            if (SlotState[slot] == SLOT_RESERVED) {
                IPC_ReserveStop(slot);          /* 예약이 실제 입차로 완료됨 */
            }
            SlotState[slot] = SLOT_OCCUPIED;    /* RESERVED -> OCCUPIED 확정 포함 */
        }
    } else {
        OccCount[slot] = 0u;
        if (EmpCount[slot] < APP_SENSOR_HYST_CNT) {
            EmpCount[slot]++;
        }
        /* 예약 중인 칸은 센서가 비었다고 해도 비우지 않는다 (예약 타이머가 해제 담당). */
        if ((EmpCount[slot] >= APP_SENSOR_HYST_CNT) && (SlotState[slot] != SLOT_RESERVED)) {
            SlotState[slot] = SLOT_EMPTY;
        }
    }
    Manager_RecountFree();
}

/* 입차 요청: 빈칸이 있으면 예약 후 차단기 개방, 없으면 거부. */
static void Manager_OnEntrance(void)
{
    OS_ERR     err;
    CPU_INT08U i;

    IPC_PostLog(LOG_ENTRANCE_DETECTED);

    if (FreeCount > 0u) {
        for (i = 0u; i < APP_SLOT_COUNT; i++) {
            if (SlotState[i] == SLOT_EMPTY) {
                SlotState[i]   = SLOT_RESERVED; /* 마지막 한 자리 중복 허용 방지 */
                OccCount[i]    = 0u;            /* 입차 확정 판정을 처음부터 새로 센다 */
                EmpCount[i]    = 0u;
                IPC_ReserveStart(i);            /* 입차 미확인 시 자동 해제 */
                break;
            }
        }
        Manager_RecountFree();
        Manager_SendGate(GATE_OPEN);
        GateOpen = DEF_TRUE;
        OSTmrStart(&GateTimer, &err);           /* N초 후 자동 닫힘 */
        IPC_PostLog(LOG_GATE_OPEN);
    } else {
        Manager_SendGate(GATE_DENY);            /* 만차 */
        IPC_PostLog(LOG_FULL_DENIED);
    }
}

static void Manager_OnGateTimeout(void)
{
    Manager_SendGate(GATE_CLOSE);
    GateOpen = DEF_FALSE;
    IPC_PostLog(LOG_GATE_CLOSE);
}

static void Manager_OnExit(void)
{
    /* 버튼은 상태를 직접 비우지 않는다. 출차 이벤트만 기록. 빈자리는 초음파로만 갱신. */
    IPC_PostLog(LOG_EXIT);
}

/* 예약 타임아웃: 입차를 허용했지만 차가 실제로 들어오지 않은 칸을 다시 비운다.
 * 이미 OCCUPIED로 확정된 경우(만료 직전 입차 완료)는 무시한다. */
static void Manager_OnReserveTimeout(CPU_INT08U slot)
{
    if (slot >= APP_SLOT_COUNT) {
        return;
    }
    if (SlotState[slot] == SLOT_RESERVED) {
        SlotState[slot] = SLOT_EMPTY;
        OccCount[slot]  = 0u;
        EmpCount[slot]  = 0u;
        Manager_RecountFree();
        IPC_PostLog(LOG_RESERVE_EXPIRED);
    }
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
        OccCount[i]  = 0u;
        EmpCount[i]  = 0u;
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
            case MSG_SENSOR:          Manager_OnSensor(p_msg->slot_id, p_msg->distance_mm); break;
            case MSG_ENTRANCE:        Manager_OnEntrance();                                 break;
            case MSG_EXIT:            Manager_OnExit();                                     break;
            case MSG_GATE_TIMEOUT:    Manager_OnGateTimeout();                              break;
            case MSG_RESERVE_TIMEOUT: Manager_OnReserveTimeout(p_msg->slot_id);             break;
            default:                                                                        break;
        }

        IPC_MsgFree(p_msg);

        Manager_SendDisplay();                  /* 매 이벤트 후 화면 갱신 요청 */
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
