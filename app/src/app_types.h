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

#define  APP_SLOT_COUNT                 4u      /* 주차칸 수 (배열 기반). LCD 표시는 6칸까지 한 줄에 가능 */
#define  APP_OCCUPIED_THRESHOLD_MM      150u    /* 이 거리(mm) 미만이면 점유로 판단                     */
#define  APP_DISTANCE_INVALID           0xFFFFu /* 측정 실패 (드라이버 INVALID를 이 값으로 정규화)      */
#define  APP_SENSOR_HYST_CNT            3u      /* 같은 판정이 N회 연속이어야 상태를 바꾼다(떨림 방지) */
#define  APP_GATE_OPEN_SEC              5u      /* 차단기를 연 뒤 자동으로 닫기까지의 시간 N(초)        */
#define  APP_RESERVE_TIMEOUT_SEC        10u     /* 예약 후 이 시간(초) 내 입차 미확인 시 예약 해제      */

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
    MSG_GATE_TIMEOUT,                           /* Gate OSTmr: 차단기 개방 후 N초 경과                 */
    MSG_RESERVE_TIMEOUT                         /* Reserve OSTmr: 예약 후 입차 미확인 (slot_id 사용)   */
} MANAGER_MSG_TYPE;

/* aligned(4): OS_MEM 풀은 블록 크기가 4의 배수여야 한다(-fshort-enums로 sizeof가 어긋나는 것 방지). */
typedef struct {
    MANAGER_MSG_TYPE  type;
    CPU_INT08U        slot_id;                  /* MSG_SENSOR/RESERVE: 대상 주차칸 번호                */
    CPU_INT16U        distance_mm;              /* MSG_SENSOR: 측정 거리(mm)                           */
} __attribute__((aligned(4))) MANAGER_MSG;

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

/*
*********************************************************************************************************
* DISPLAY MESSAGE (ParkingManager -> DisplayTask)
*********************************************************************************************************
*/

/* aligned(4): 위와 동일. slot 수에 따라 sizeof가 4의 배수가 아니게 되는 것을 막는다. */
typedef struct {
    CPU_INT08U   free_count;                    /* 빈자리 수                                           */
    CPU_BOOLEAN  gate_open;                     /* 차단기 열림 여부                                    */
    SLOT_STATE   slot[APP_SLOT_COUNT];          /* 각 주차칸 상태                                      */
} __attribute__((aligned(4))) DISPLAY_MSG;

/*
*********************************************************************************************************
* LOG EVENT (ParkingManager -> AlarmLogTask)
*   값(enum)을 그대로 큐 포인터에 인코딩해 전달한다. 로그 문자열/부저는 AlarmLogTask가 결정한다.
*********************************************************************************************************
*/

typedef enum {
    LOG_ENTRANCE_DETECTED = 0,                  /* "[EVENT] entrance_detected"                         */
    LOG_GATE_OPEN,                              /* "[GATE] open"                                       */
    LOG_GATE_CLOSE,                             /* "[GATE] close"                                      */
    LOG_FULL_DENIED,                            /* "[ALARM] parking_full entrance_denied" + 부저       */
    LOG_EXIT,                                   /* "[BUTTON] exit_event_logged"                        */
    LOG_RESERVE_EXPIRED                         /* "[EVENT] reservation_expired" (입차 미확인 해제)    */
} LOG_EVENT;

#endif  /* APP_TYPES_H */
