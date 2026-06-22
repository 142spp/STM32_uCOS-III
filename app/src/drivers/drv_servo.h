/*
*********************************************************************************************************
* Smart Parking - drv_servo.h
*
* 입구 차단기용 서보모터 드라이버 (TIM PWM, 50Hz). RTOS 비의존.
* 서보는 별도 5V 전원으로 구동하며 GND는 STM32와 공통으로 연결한다.
*********************************************************************************************************
*/

#ifndef  DRV_SERVO_H
#define  DRV_SERVO_H

void  Servo_Init  (void);                       /* TIM PWM 50Hz 초기화                                */
void  Servo_Open  (void);                       /* 차단기 열림 각도로 이동                            */
void  Servo_Close (void);                       /* 차단기 닫힘 각도로 이동                            */

#endif  /* DRV_SERVO_H */
