/*
*********************************************************************************************************
* Smart Parking - app_types.h
*
* 여러 task가 공유하는 타입, 메시지 포맷, 시스템 상수, task 우선순위를 한 곳에 모은다.
* 이 헤더는 로직(tasks)만 포함하며, 드라이버(drivers)는 포함하지 않는다.
*********************************************************************************************************
*/

#ifndef  APP_TYPES_H
#define  APP_TYPES_H

#include  <includes.h>

/*
*********************************************************************************************************
* SYSTEM CONFIG (튜닝 대상)
*********************************************************************************************************
*/

#define  APP_SLOT_COUNT                 2u      /* 기본 주차칸 수 (배열 기반, 4칸까지 확장)            */
#define  APP_OCCUPIED_THRESHOLD_MM      150u    /* 이 거리(mm) 미만이면 점유로 판단 (히스테리시스는 구현 시) */
#define  APP_GATE_OPEN_SEC              5u      /* 차단기를 연 뒤 자동으로 닫기까지의 시간 N(초)        */

/*
*********************************************************************************************************
* TASK PRIORITIES (값이 작을수록 우선순위 높음, OS_CFG_PRIO_MAX = 64)
*********************************************************************************************************
*/

#define  PRIO_PARKING_MANAGER           4u
#define  PRIO_GATE_CONTROL              5u
#define  PRIO_SLOT_SENSOR               6u
#define  PRIO_ENTRANCE                  7u
#define  PRIO_BUTTON                    8u
#define  PRIO_DISPLAY                   9u
#define  PRIO_ALARM_LOG                 10u

#define  APP_TASK_STK_SIZE              APP_CFG_TASK_START_STK_SIZE

/*
*********************************************************************************************************
* SLOT STATE
*********************************************************************************************************
*/

typedef enum {
    SLOT_EMPTY = 0,                             /* 빈자리                                              */
    SLOT_OCCUPIED,                              /* 초음파로 점유 확정                                  */
    SLOT_RESERVED                               /* 입차 허용되어 잠금, 점유 확정 전 (중복 입차 방지)   */
} SLOT_STATE;

/*
*********************************************************************************************************
* MANAGER MESSAGE
*
* ParkingManagerTask로 들어오는 모든 입력은 단일 큐(ManagerQueue)로 모이며, type으로 구분한다.
*********************************************************************************************************
*/

typedef enum {
    MSG_SENSOR = 0,                             /* SlotSensorTask: 측정값                              */
    MSG_ENTRANCE,                               /* EntranceTask: 입구 차량 접근                        */
    MSG_EXIT,                                   /* ButtonTask: 출차/관리자 이벤트                      */
    MSG_GATE_TIMEOUT                            /* Gate OSTmr: 차단기 개방 후 N초 경과                 */
} MANAGER_MSG_TYPE;

typedef struct {
    MANAGER_MSG_TYPE  type;
    CPU_INT08U        slot_id;                  /* MSG_SENSOR: 대상 주차칸 번호                        */
    CPU_INT16U        distance_mm;              /* MSG_SENSOR: 측정 거리(mm)                           */
} MANAGER_MSG;

/*
*********************************************************************************************************
* GATE COMMAND (ParkingManager -> GateControlTask)
*********************************************************************************************************
*/

typedef enum {
    GATE_OPEN = 0,
    GATE_CLOSE,
    GATE_DENY                                   /* 만차: 차단기 유지 + 경고                            */
} GATE_CMD;

#endif  /* APP_TYPES_H */
