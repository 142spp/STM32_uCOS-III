/*
*********************************************************************************************************
* Smart Parking - task_slot_sensor.c
*
* HC-SR04 센서를 순차적으로 측정하고, 측정 거리를 ManagerQueue(MSG_SENSOR)로 전달한다.
*********************************************************************************************************
*/

#include  "task_slot_sensor.h"
#include  "app_types.h"
#include  "ipc.h"
#include  "drv_hcsr04.h"

static  OS_TCB   SlotSensorTCB;
static  CPU_STK  SlotSensorStk[APP_TASK_STK_SIZE];

static void SlotSensorTask(void *p_arg)
{
    OS_ERR       err;
    CPU_INT08U   slot;
    CPU_INT16U   dist;
    MANAGER_MSG *p_msg;

    (void)p_arg;

    HCSR04_Init();

    while (DEF_TRUE) {
        /* 센서 간섭을 막기 위해 한 번에 한 칸씩 trigger -> echo 대기 -> 읽기. */
        for (slot = 0u; slot < APP_SLOT_COUNT; slot++) {
            HCSR04_Trigger(slot);
            OSTimeDlyHMSM(0u, 0u, 0u, 60u, OS_OPT_TIME_HMSM_STRICT, &err);   /* echo 왕복 + 잔향 여유 */

            dist = HCSR04_ReadDistance(slot);
            if (dist == HCSR04_DISTANCE_INVALID) {
                dist = APP_DISTANCE_INVALID;    /* 드라이버 센티넬을 app 레이어 값으로 정규화 */
            }

            p_msg = IPC_MsgAlloc();
            if (p_msg != (MANAGER_MSG *)0) {
                p_msg->type        = MSG_SENSOR;
                p_msg->slot_id     = slot;
                p_msg->distance_mm = dist;
                IPC_PostManager(p_msg);
            }
        }
        OSTimeDlyHMSM(0u, 0u, 0u, 200u, OS_OPT_TIME_HMSM_STRICT, &err);      /* 칸 한 바퀴 후 측정 주기 */
    }
}

void SlotSensorTask_Create(void)
{
    OS_ERR  err;

    OSTaskCreate(&SlotSensorTCB,
                 "Slot Sensor Task",
                 SlotSensorTask,
                 (void *)0,
                 PRIO_SLOT_SENSOR,
                 &SlotSensorStk[0],
                 APP_TASK_STK_SIZE / 10u,
                 APP_TASK_STK_SIZE,
                 0u,
                 0u,
                 (void *)0,
                 OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR,
                 &err);
}
