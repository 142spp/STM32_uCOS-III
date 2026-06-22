/*
*********************************************************************************************************
* Smart Parking - task_gate_control.c
*
* GateCommandQueue로 받은 명령(GATE_OPEN/CLOSE/DENY)에 따라 서보모터 차단기를 제어한다.
* 차단기는 GateControlTask만 물리적으로 만지므로 별도 락이 필요 없다.
*********************************************************************************************************
*/

#include  "task_gate_control.h"
#include  "app_types.h"
#include  "ipc.h"
#include  "drv_servo.h"

static  OS_TCB   GateControlTCB;
static  CPU_STK  GateControlStk[APP_TASK_STK_SIZE];

static void GateControlTask(void *p_arg)
{
    OS_ERR       err;
    OS_MSG_SIZE  size;
    void        *p_data;
    GATE_CMD     cmd;

    (void)p_arg;

    Servo_Init();
    Servo_Close();

    while (DEF_TRUE) {
        p_data = OSQPend(&GateCommandQueue, 0u, OS_OPT_PEND_BLOCKING, &size, (CPU_TS *)0, &err);
        if (err != OS_ERR_NONE) {
            continue;
        }
        cmd = (GATE_CMD)(CPU_ADDR)p_data;       /* 작은 스칼라는 포인터에 직접 인코딩 */

        switch (cmd) {
            case GATE_OPEN:   Servo_Open();  break;
            case GATE_CLOSE:  Servo_Close(); break;
            case GATE_DENY:   /* 차단기 유지. 경고는 AlarmLogTask가 담당. */ break;
            default:                          break;
        }
    }
}

void GateControlTask_Create(void)
{
    OS_ERR  err;

    OSTaskCreate(&GateControlTCB,
                 "Gate Control Task",
                 GateControlTask,
                 (void *)0,
                 PRIO_GATE_CONTROL,
                 &GateControlStk[0],
                 APP_TASK_STK_SIZE / 10u,
                 APP_TASK_STK_SIZE,
                 0u,
                 0u,
                 (void *)0,
                 OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR,
                 &err);
}
