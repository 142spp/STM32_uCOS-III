/*
*********************************************************************************************************
* Smart Parking - drv_led.c
*
* 4핀 RGB LED (공통 캐소드). 공통핀이 GND이므로 각 채널을 HIGH로 두면 켜진다(active-high).
*********************************************************************************************************
*/

#include  "drv_led.h"
#include  "stm32f4xx.h"
#include  "stm32f4xx_rcc.h"
#include  "stm32f4xx_gpio.h"

#define  LED_PORT     GPIOF
#define  LED_R_PIN    GPIO_Pin_13               /* Arduino D7  */
#define  LED_G_PIN    GPIO_Pin_14               /* Arduino D4  */
#define  LED_B_PIN    GPIO_Pin_15               /* Arduino D2  */

void LED_Init(void)
{
    GPIO_InitTypeDef gpio;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOF, ENABLE);

    gpio.GPIO_Pin   = LED_R_PIN | LED_G_PIN | LED_B_PIN;
    gpio.GPIO_Mode  = GPIO_Mode_OUT;
    gpio.GPIO_OType = GPIO_OType_PP;
    gpio.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(LED_PORT, &gpio);

    GPIO_ResetBits(LED_PORT, LED_R_PIN | LED_G_PIN | LED_B_PIN);   /* 전부 끔 */
}

void LED_SetRGB(uint8_t r, uint8_t g, uint8_t b)
{
    GPIO_WriteBit(LED_PORT, LED_R_PIN, (r != 0u) ? Bit_SET : Bit_RESET);
    GPIO_WriteBit(LED_PORT, LED_G_PIN, (g != 0u) ? Bit_SET : Bit_RESET);
    GPIO_WriteBit(LED_PORT, LED_B_PIN, (b != 0u) ? Bit_SET : Bit_RESET);
}

void LED_Set(LED_COLOR color)
{
    switch (color) {
        case LED_GREEN:  LED_SetRGB(0u, 1u, 0u); break;
        case LED_YELLOW: LED_SetRGB(1u, 1u, 0u); break;   /* 빨강+초록 = 노랑 */
        case LED_RED:    LED_SetRGB(1u, 0u, 0u); break;
        case LED_OFF:
        default:         LED_SetRGB(0u, 0u, 0u); break;
    }
}
