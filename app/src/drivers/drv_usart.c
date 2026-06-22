/*
*********************************************************************************************************
* Smart Parking - drv_usart.c
*********************************************************************************************************
*/

#include  "drv_usart.h"
#include  "stm32f4xx.h"
#include  "stm32f4xx_rcc.h"
#include  "stm32f4xx_gpio.h"
#include  "stm32f4xx_usart.h"

void Usart_Init(void)
{
    GPIO_InitTypeDef  gpio_init;
    USART_InitTypeDef usart_init;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);

    GPIO_PinAFConfig(GPIOD, GPIO_PinSource8, GPIO_AF_USART3);
    GPIO_PinAFConfig(GPIOD, GPIO_PinSource9, GPIO_AF_USART3);

    gpio_init.GPIO_Pin   = GPIO_Pin_8 | GPIO_Pin_9;
    gpio_init.GPIO_Mode  = GPIO_Mode_AF;
    gpio_init.GPIO_OType = GPIO_OType_PP;
    gpio_init.GPIO_PuPd  = GPIO_PuPd_UP;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOD, &gpio_init);

    usart_init.USART_BaudRate            = 115200u;
    usart_init.USART_WordLength          = USART_WordLength_8b;
    usart_init.USART_StopBits            = USART_StopBits_1;
    usart_init.USART_Parity              = USART_Parity_No;
    usart_init.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    usart_init.USART_Mode                = USART_Mode_Rx | USART_Mode_Tx;

    USART_Init(USART3, &usart_init);
    USART_Cmd(USART3, ENABLE);
}

void Usart_PutChar(char ch)
{
    while (USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET) {
    }
    USART_SendData(USART3, (uint16_t)ch);
}

void Usart_PutStr(const char *p_str)
{
    if (p_str == 0) {
        return;
    }
    while (*p_str != '\0') {
        Usart_PutChar(*p_str);
        p_str++;
    }
}

void Usart_PutUInt(uint32_t value)
{
    char     buf[10];                           /* uint32_t 최대 10자리                               */
    uint8_t  ix = 0u;

    if (value == 0u) {
        Usart_PutChar('0');
        return;
    }

    while ((value > 0u) && (ix < sizeof(buf))) {
        buf[ix] = (char)('0' + (value % 10u));
        value /= 10u;
        ix++;
    }

    while (ix > 0u) {
        ix--;
        Usart_PutChar(buf[ix]);
    }
}
