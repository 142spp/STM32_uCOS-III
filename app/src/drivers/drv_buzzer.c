/*
*********************************************************************************************************
* Smart Parking - drv_buzzer.c
*
* 수동 부저 경고음. TIM4_CH1(PD12, AF2)로 가청 주파수(~2kHz) PWM 출력. RTOS 비의존.
*   수동 부저는 단순 GPIO HIGH로는 소리가 안 나므로 PWM으로 구동한다.
*   (능동 부저라면 GPIO On/Off로 대체 가능)
*********************************************************************************************************
*/

#include  "drv_buzzer.h"
#include  "stm32f4xx.h"
#include  "stm32f4xx_rcc.h"
#include  "stm32f4xx_gpio.h"
#include  "stm32f4xx_tim.h"

/* APB1 타이머(TIM4) 84MHz -> /84 = 1MHz(1us/tick), 주기 500us = 2kHz */
#define  BUZZER_TIM_PSC        (84u - 1u)
#define  BUZZER_PERIOD_US      500u

void Buzzer_Init(void)
{
    GPIO_InitTypeDef        gpio;
    TIM_TimeBaseInitTypeDef tb;
    TIM_OCInitTypeDef       oc;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

    gpio.GPIO_Pin   = GPIO_Pin_12;
    gpio.GPIO_Mode  = GPIO_Mode_AF;
    gpio.GPIO_OType = GPIO_OType_PP;
    gpio.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOD, &gpio);
    GPIO_PinAFConfig(GPIOD, GPIO_PinSource12, GPIO_AF_TIM4);

    tb.TIM_Prescaler         = BUZZER_TIM_PSC;
    tb.TIM_CounterMode       = TIM_CounterMode_Up;
    tb.TIM_Period            = BUZZER_PERIOD_US - 1u;
    tb.TIM_ClockDivision     = TIM_CKD_DIV1;
    tb.TIM_RepetitionCounter = 0u;
    TIM_TimeBaseInit(TIM4, &tb);

    oc.TIM_OCMode      = TIM_OCMode_PWM1;
    oc.TIM_OutputState = TIM_OutputState_Enable;
    oc.TIM_OCPolarity  = TIM_OCPolarity_High;
    oc.TIM_Pulse       = 0u;                     /* 시작은 무음 (duty 0) */
    TIM_OC1Init(TIM4, &oc);
    TIM_OC1PreloadConfig(TIM4, TIM_OCPreload_Enable);

    TIM_Cmd(TIM4, ENABLE);
}

void Buzzer_On(void)
{
    TIM_SetCompare1(TIM4, BUZZER_PERIOD_US / 2u);   /* 50% duty -> 소리 */
}

void Buzzer_Off(void)
{
    TIM_SetCompare1(TIM4, 0u);                      /* duty 0 -> 무음 */
}
