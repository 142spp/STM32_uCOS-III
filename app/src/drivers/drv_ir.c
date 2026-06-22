/*
*********************************************************************************************************
* Smart Parking - drv_ir.c
*
* 적외선 장애물 감지 센서 (디지털 입력). 입구 차량 접근 감지용. RTOS 비의존.
*   DO : PC0
* 대부분의 IR 장애물 모듈은 감지 시 출력이 LOW(active-low). 실제 모듈에 맞게 IR_DETECTED_LEVEL 조정.
*********************************************************************************************************
*/

#include  "drv_ir.h"
#include  "stm32f4xx.h"
#include  "stm32f4xx_rcc.h"
#include  "stm32f4xx_gpio.h"

#define  IR_PORT             GPIOC
#define  IR_PIN              GPIO_Pin_0
#define  IR_DETECTED_LEVEL   Bit_RESET           /* 감지 시 핀 레벨 (active-low)                       */

void IR_Init(void)
{
    GPIO_InitTypeDef gpio;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

    gpio.GPIO_Pin   = IR_PIN;
    gpio.GPIO_Mode  = GPIO_Mode_IN;
    gpio.GPIO_OType = GPIO_OType_PP;
    gpio.GPIO_PuPd  = GPIO_PuPd_UP;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(IR_PORT, &gpio);
}

uint8_t IR_IsDetected(void)
{
    return (GPIO_ReadInputDataBit(IR_PORT, IR_PIN) == IR_DETECTED_LEVEL) ? 1u : 0u;
}
