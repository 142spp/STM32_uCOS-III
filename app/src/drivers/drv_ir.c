/*
*********************************************************************************************************
* Smart Parking - drv_ir.c
*********************************************************************************************************
*/

#include  "drv_ir.h"
#include  "stm32f4xx.h"
#include  "stm32f4xx_rcc.h"
#include  "stm32f4xx_gpio.h"

void IR_Init(void)
{
    /* TODO: IR 센서 출력 핀을 GPIO 입력으로 설정 (모듈에 따라 active-low일 수 있음) */
}

uint8_t IR_IsDetected(void)
{
    /* TODO: 핀 레벨을 읽어 감지 여부 반환 */
    return 0u;
}
