/*
*********************************************************************************************************
* Smart Parking - drv_buzzer.h
*
* 수동 부저 드라이버 (GPIO 또는 TIM PWM). RTOS 비의존.
*********************************************************************************************************
*/

#ifndef  DRV_BUZZER_H
#define  DRV_BUZZER_H

void  Buzzer_Init (void);
void  Buzzer_On   (void);                        /* 경고음 출력 시작                                   */
void  Buzzer_Off  (void);                        /* 경고음 정지                                        */

#endif  /* DRV_BUZZER_H */
