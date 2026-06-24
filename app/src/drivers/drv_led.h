/*
*********************************************************************************************************
* Smart Parking - drv_led.h
*
* 4핀 RGB LED (공통 캐소드 = 공통 GND). R/G/B 각 채널을 GPIO로 on/off (active-high).
*   R=PF13(D7), G=PF14(D4), B=PF15(D2), 공통핀 -> GND. 각 채널에 직렬 저항 필요.
* 입차 가능/예약/만차 상태 표시용. RTOS 비의존.
*********************************************************************************************************
*/

#ifndef  DRV_LED_H
#define  DRV_LED_H

#include  <stdint.h>

typedef enum {
    LED_OFF = 0,
    LED_GREEN,                                  /* 빈자리 있음                                          */
    LED_YELLOW,                                 /* 예약중 (R+G)                                         */
    LED_RED                                     /* 만차                                                 */
} LED_COLOR;

void  LED_Init   (void);
void  LED_SetRGB (uint8_t r, uint8_t g, uint8_t b);   /* 각 채널 on(1)/off(0) - 검증/임의 색용          */
void  LED_Set    (LED_COLOR color);                   /* 의미 있는 색상 한 번에 설정                    */

#endif  /* DRV_LED_H */
