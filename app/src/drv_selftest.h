/*
*********************************************************************************************************
* Smart Parking - drv_selftest.h
*
* 드라이버 단독 검증용 하네스. RTOS 로직(ParkingApp) 대신 이 루틴 하나만 돌려
* 부품을 하나씩 살린다. 검증이 끝나면 app.c의 APP_SELFTEST를 0으로 되돌린다.
*********************************************************************************************************
*/

#ifndef  DRV_SELFTEST_H
#define  DRV_SELFTEST_H

/* AppTaskStart(task 컨텍스트)에서 호출. 무한 루프이며 리턴하지 않는다. */
void  DrvSelfTest_Run(void);

#endif  /* DRV_SELFTEST_H */
