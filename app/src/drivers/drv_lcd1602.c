/*
*********************************************************************************************************
* Smart Parking - drv_lcd1602.c
*********************************************************************************************************
*/

#include  "drv_lcd1602.h"
#include  "stm32f4xx.h"
#include  "stm32f4xx_rcc.h"
#include  "stm32f4xx_gpio.h"
#include  "stm32f4xx_i2c.h"

void LCD_Init(void)
{
    /* TODO: I2C 초기화 + HD44780 4비트 초기화 시퀀스. 백팩 주소(0x27/0x3F) 확인. */
}

void LCD_Clear(void)
{
    /* TODO: clear display 명령(0x01) 전송 */
}

void LCD_Print(uint8_t row, const char *p_str)
{
    (void)row;
    (void)p_str;
    /* TODO: 커서를 해당 행 시작으로 이동 후 문자열 전송 (최대 16자) */
}
