/*
*********************************************************************************************************
* Smart Parking - app_parking.h
*
* Smart Parking 시스템의 단일 통합 지점.
* 모든 IPC 객체와 task를 한 번에 생성한다. 전환 시 AppTaskStart에서 이 함수만 호출하면 된다.
*********************************************************************************************************
*/

#ifndef  APP_PARKING_H
#define  APP_PARKING_H

/* IPC 객체 생성 + 전체 task 생성 (OS 시작 후, 최초 task 안에서 호출) */
void  ParkingApp_Init(void);

#endif  /* APP_PARKING_H */
