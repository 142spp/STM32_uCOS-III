/*
*********************************************************************************************************
* Smart Parking - drv_hcsr04.h
*
* HC-SR04 초음파 거리 센서 드라이버. echo 펄스 폭은 타이머 입력 캡처로 측정한다.
* 이 드라이버는 uC/OS-III를 알지 못한다 (RTOS 비의존).
*********************************************************************************************************
*/

#ifndef  DRV_HCSR04_H
#define  DRV_HCSR04_H

#include  <stdint.h>

#define  HCSR04_DISTANCE_INVALID   0xFFFFu      /* 측정 실패/타임아웃                                  */

void      HCSR04_Init        (void);                    /* GPIO/TIM 입력 캡처 초기화                   */
void      HCSR04_Trigger     (uint8_t sensor_id);       /* 해당 센서 trigger 펄스 출력                 */
uint16_t  HCSR04_ReadDistance(uint8_t sensor_id);       /* 최근 측정 거리(mm), 실패 시 INVALID         */

#endif  /* DRV_HCSR04_H */
