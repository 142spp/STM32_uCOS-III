/*
*********************************************************************************************************
* Smart Parking - drv_servo.c
*
* 입구 차단기 서보모터. TIM3_CH1(PA6, AF2)로 50Hz PWM 출력.
*   주기 20ms, 펄스 1.0~2.0ms = 0~180도. (HC-SR04가 TIM2를 쓰므로 서보는 TIM3 사용)
* 서보는 별도 5V 전원으로 구동하고 GND는 STM32와 공통으로 연결한다.
*********************************************************************************************************
*/

#include  "drv_servo.h"
#include  "stm32f4xx.h"
#include  "stm32f4xx_rcc.h"
#include  "stm32f4xx_gpio.h"
#include  "stm32f4xx_tim.h"

/* TIM3 입력 클럭 90MHz 가정 -> 1MHz(1us/tick), 주기 20000us = 50Hz */
#define  SERVO_TIM_PSC          (90u - 1u)
#define  SERVO_TIM_PERIOD_US    20000u

/* 펄스 폭(us). 실제 차단기 기구에 맞춰 보정한다. */
#define  SERVO_PULSE_CLOSE_US   1000u           /* 닫힘 (~0도)                                         */
#define  SERVO_PULSE_OPEN_US    2000u           /* 열림 (~180도)                                       */

void Servo_Init(void)
{
    GPIO_InitTypeDef        gpio;
    TIM_TimeBaseInitTypeDef tb;
    TIM_OCInitTypeDef       oc;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

    gpio.GPIO_Pin   = GPIO_Pin_6;
    gpio.GPIO_Mode  = GPIO_Mode_AF;
    gpio.GPIO_OType = GPIO_OType_PP;
    gpio.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOA, &gpio);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource6, GPIO_AF_TIM3);

    tb.TIM_Prescaler         = SERVO_TIM_PSC;
    tb.TIM_CounterMode       = TIM_CounterMode_Up;
    tb.TIM_Period            = SERVO_TIM_PERIOD_US - 1u;
    tb.TIM_ClockDivision     = TIM_CKD_DIV1;
    tb.TIM_RepetitionCounter = 0u;
    TIM_TimeBaseInit(TIM3, &tb);

    oc.TIM_OCMode      = TIM_OCMode_PWM1;
    oc.TIM_OutputState = TIM_OutputState_Enable;
    oc.TIM_OCPolarity  = TIM_OCPolarity_High;
    oc.TIM_Pulse       = SERVO_PULSE_CLOSE_US;   /* 시작은 닫힘 */
    TIM_OC1Init(TIM3, &oc);
    TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);

    TIM_Cmd(TIM3, ENABLE);
}

void Servo_Open(void)
{
    TIM_SetCompare1(TIM3, SERVO_PULSE_OPEN_US);
}

void Servo_Close(void)
{
    TIM_SetCompare1(TIM3, SERVO_PULSE_CLOSE_US);
}
