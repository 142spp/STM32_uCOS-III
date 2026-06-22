/*
*********************************************************************************************************
* Smart Parking - drv_usart.h
*
* USART3 로그 출력 드라이버 (PD8 TX / PD9 RX, 115200 8N1). RTOS 비의존, 폴링 송신.
*
* 사용 규칙: 부팅 로그는 app.c가, 이벤트 로그는 AlarmLogTask가 호출한다.
*           OS 시작 후에는 AlarmLogTask 한 곳으로 출력을 모아 로그가 뒤섞이지 않게 한다.
*********************************************************************************************************
*/

#ifndef  DRV_USART_H
#define  DRV_USART_H

#include  <stdint.h>

void  Usart_Init    (void);                     /* USART3 + GPIO 초기화 (부팅 시 1회)                 */
void  Usart_PutChar (char ch);                  /* 한 문자 송신 (TXE 폴링)                            */
void  Usart_PutStr  (const char *p_str);        /* 널 종료 문자열 송신                                 */
void  Usart_PutUInt (uint32_t value);           /* 부호 없는 10진수 송신                               */

#endif  /* DRV_USART_H */
