/*
*********************************************************************************************************
* Smart Parking - drv_servo.c
*********************************************************************************************************
*/

#include  "drv_servo.h"
#include  "stm32f4xx.h"
#include  "stm32f4xx_rcc.h"
#include  "stm32f4xx_gpio.h"
#include  "stm32f4xx_tim.h"

void Servo_Init(void)
{
    /* TODO: TIM PWM 출력 50Hz(주기 20ms) 설정. 1.0~2.0ms 펄스 = 0~180도. */
}

void Servo_Open(void)
{
    /* TODO: 열림 각도에 해당하는 CCR 값 설정 */
}

void Servo_Close(void)
{
    /* TODO: 닫힘 각도에 해당하는 CCR 값 설정 */
}
