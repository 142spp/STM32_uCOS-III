#include  <includes.h>
#include  "serial.h"

#include  "stm32f4xx_gpio.h"
#include  "stm32f4xx_rcc.h"
#include  "stm32f4xx_usart.h"
#include  "stm32f4xx.h"

/*
*********************************************************************************************************
*                                          Serial_Init()
*
* Description : Configure USART3 for the NUCLEO ST-LINK virtual COM port.
*
* Note(s)     : NUCLEO-F429ZI VCP uses USART3 PD8(TX) / PD9(RX).
*********************************************************************************************************
*/
void Serial_Init(void)
{
    GPIO_InitTypeDef  gpio_init  = {0};
    USART_InitTypeDef usart_init = {0};

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

    USART_StructInit(&usart_init);
    usart_init.USART_BaudRate            = 115200u;
    usart_init.USART_WordLength          = USART_WordLength_8b;
    usart_init.USART_StopBits            = USART_StopBits_1;
    usart_init.USART_Parity              = USART_Parity_No;
    usart_init.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    usart_init.USART_Mode                = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(USART3, &usart_init);
    USART_Cmd(USART3, ENABLE);

    Serial_WriteString("\r\nUSART3 serial monitor ready\r\n");
}

CPU_BOOLEAN Serial_ReadChar(char *ch)
{
    if (USART_GetFlagStatus(USART3, USART_FLAG_RXNE) == RESET) {
        return DEF_FALSE;
    }

    *ch = (char)(USART_ReceiveData(USART3) & 0xFFu);
    return DEF_TRUE;
}

void Serial_WriteChar(char ch)
{
    while (USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET) {
    }

    USART_SendData(USART3, (uint16_t)ch);
}

void Serial_WriteString(const char *str)
{
    while (*str != '\0') {
        Serial_WriteChar(*str++);
    }
}
