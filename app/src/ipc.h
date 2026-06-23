/*
*********************************************************************************************************
* Smart Parking - ipc.h
*
* task 간 통신에 쓰이는 OS 객체(큐, 타이머, 메시지 풀)를 선언한다.
* IPC_Init()으로 한 번에 생성하며, task들은 여기 선언된 객체를 공유한다.
*********************************************************************************************************
*/

#ifndef  IPC_H
#define  IPC_H

#include  <includes.h>
#include  "app_types.h"

/* ParkingManager로 들어오는 모든 입력을 모으는 단일 큐 */
extern  OS_Q     ManagerQueue;

/* ParkingManager가 내보내는 명령/표시/로그 큐 */
extern  OS_Q     GateCommandQueue;
extern  OS_Q     DisplayQueue;
extern  OS_Q     LogQueue;

/* 차단기 자동 닫힘 타이머 (만료 시 ManagerQueue로 MSG_GATE_TIMEOUT 전송) */
extern  OS_TMR   GateTimer;

/* 칸별 예약 타임아웃 타이머 (만료 시 ManagerQueue로 MSG_RESERVE_TIMEOUT 전송) */
extern  OS_TMR   ReserveTimer[APP_SLOT_COUNT];

/* 모든 OS 객체 생성 */
void         IPC_Init        (void);

/* ManagerQueue 메시지 헬퍼 (메시지 풀 기반) */
MANAGER_MSG *IPC_MsgAlloc    (void);                    /* 풀에서 메시지 블록 할당 (실패 시 0)         */
void         IPC_MsgFree      (MANAGER_MSG *p_msg);      /* 처리 완료 후 풀로 반환                       */
void         IPC_PostManager  (MANAGER_MSG *p_msg);      /* ManagerQueue로 전송                          */

/* DisplayQueue 메시지 헬퍼 (별도 풀) */
DISPLAY_MSG *IPC_DisplayAlloc (void);
void         IPC_DisplayFree  (DISPLAY_MSG *p_msg);
void         IPC_PostDisplay  (DISPLAY_MSG *p_msg);

/* LogQueue: LOG_EVENT 코드를 포인터에 인코딩해 전송 (풀 불필요) */
void         IPC_PostLog      (LOG_EVENT event);

/* 칸별 예약 타임아웃 타이머 제어 (입차 허용 시 시작, 점유 확정 시 정지) */
void         IPC_ReserveStart (CPU_INT08U slot);
void         IPC_ReserveStop  (CPU_INT08U slot);

#endif  /* IPC_H */
