/*
*********************************************************************************************************
* Smart Parking - app_parking.c
*
* IPC 객체와 모든 task를 생성하는 통합 지점.
*
* 사용법: 현재 app.c는 HW#6(LED 게임)이다. Smart Parking으로 전환할 때
*         AppTaskStart 내부의 LED 게임 task 생성 코드를 ParkingApp_Init() 호출로 교체한다.
*********************************************************************************************************
*/

#include  "app_parking.h"
#include  "ipc.h"

#include  "task_slot_sensor.h"
#include  "task_entrance.h"
#include  "task_parking_manager.h"
#include  "task_gate_control.h"
#include  "task_display.h"
#include  "task_alarm_log.h"
#include  "task_button.h"

void ParkingApp_Init(void)
{
    IPC_Init();                                 /* 큐/타이머/메시지 풀 먼저 생성 */

    ParkingManagerTask_Create();                /* 상태 소유자 먼저 */
    SlotSensorTask_Create();
    EntranceTask_Create();
    GateControlTask_Create();
    DisplayTask_Create();
    AlarmLogTask_Create();
    ButtonTask_Create();
}
