/*
*********************************************************************************************************
* Smart Parking - drv_lcd1602.h
*
* 1602 Character LCD (I2C, PCF8574 백팩) 드라이버. RTOS 비의존.
* I2C 주소는 0x27 또는 0x3F일 수 있으므로 실제 모듈 주소를 확인한다.
*********************************************************************************************************
*/

#ifndef  DRV_LCD1602_H
#define  DRV_LCD1602_H

#include  <stdint.h>

#define  LCD1602_COLS   16u
#define  LCD1602_ROWS   2u

void  LCD_Init  (void);                                 /* I2C + HD44780 초기화                        */
void  LCD_Clear (void);                                 /* 화면 지우기                                 */
void  LCD_Print (uint8_t row, const char *p_str);       /* 지정 행(0/1)에 문자열 출력                  */

#endif  /* DRV_LCD1602_H */
