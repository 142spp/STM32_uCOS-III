/*
*********************************************************************************************************
*                                             HW#5 STARTER CODE
*
*                                  uC/OS-III USART LED Command Control
*
* Board assumption:
*   USART3 TX : PD9
*   USART3 RX : PD8
*   LED1      : PB0
*   LED2      : PB7
*   LED3      : PB14
*
* Starter code provides uC/OS startup, USART3/GPIO setup, and USART output.
*********************************************************************************************************
*/

#include <includes.h>
#include "os.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_usart.h"
#include "stm32f4xx.h"

/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

#define APP_TASK_STK_SIZE      256u
#define LED_COUNT				 3u
#define APP_TASK_USART_PRIO		10u
#define APP_TASK_LED_PRIO		11u

/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static void AppTaskStart(void *p_arg);
static void AppTaskCreate(void);

static void AppTaskUsart(void *p_arg);
static void AppTaskLed(void *p_arg);

static void Setup_Gpio(void);
static void Setup_Usart3(void);

static void UsartPutChar(char c);
static void UsartPrint(const char *s);

static void HandleCommand(char *cmd);

/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/

static OS_TCB  AppTaskStartTCB;
static CPU_STK AppTaskStartStk[APP_CFG_TASK_START_STK_SIZE];

static OS_TCB  UsartTaskTCB;
static CPU_STK UsartTaskStk[APP_TASK_STK_SIZE];

static OS_TCB  LedTaskTCB;
static CPU_STK LedTaskStk[APP_TASK_STK_SIZE];

/*
*********************************************************************************************************
*                                                main()
*********************************************************************************************************
*/

int main(void)
{
    OS_ERR err;

    RCC_DeInit();

    Setup_Gpio();
    Setup_Usart3();

    BSP_IntDisAll();

    CPU_Init();
    Mem_Init();
    Math_Init();

    OSInit(&err);

    OSTaskCreate(&AppTaskStartTCB,
                 "App Task Start",
                 AppTaskStart,
                 0u,
                 APP_CFG_TASK_START_PRIO,
                 &AppTaskStartStk[0u],
                 APP_CFG_TASK_START_STK_SIZE / 10u,
                 APP_CFG_TASK_START_STK_SIZE,
                 0u,
                 0u,
                 0u,
                 OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR,
                 &err);

    OSStart(&err);

    while (DEF_TRUE) {
    }
}

/*
*********************************************************************************************************
*                                          STARTUP TASK
*********************************************************************************************************
*/

static void AppTaskStart(void *p_arg)
{
    (void)p_arg;

    BSP_Init();
    BSP_Tick_Init();
	

    BSP_LED_Off(0u);

    UsartPrint("\r\nHW#5 USART LED Command Starter\r\n");
    UsartPrint("> ");

    AppTaskCreate();
}

/*
*********************************************************************************************************
*                                          TASK CREATION
*********************************************************************************************************
*/

static void AppTaskCreate(void)
{
	OS_ERR err;

	OSTaskCreate(&UsartTaskTCB,
                 "Usart Task",
                 AppTaskUsart,
                 0u,
                 APP_TASK_USART_PRIO,
                 &UsartTaskStk[0u],
                 APP_TASK_STK_SIZE / 10u,
                 APP_TASK_STK_SIZE,
                 0u,
                 0u,
                 0u,
                 OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR,
                 &err);

	OSTaskCreate(&LedTaskTCB,
                 "Led Task",
                 AppTaskLed,
                 0u,
                 APP_TASK_LED_PRIO,
                 &LedTaskStk[0u],
                 APP_TASK_STK_SIZE / 10u,
                 APP_TASK_STK_SIZE,
                 0u,
                 0u,
                 0u,
                 OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR,
                 &err);

}

/*
*********************************************************************************************************
*                                      USART COMMAND RECEIVE TASK
*********************************************************************************************************
*/

static void AppTaskUsart(void *p_arg)
{
    (void)p_arg;

    /*
     * TODO
     */
}

/*
*********************************************************************************************************
*                                          LED CONTROL TASK
*********************************************************************************************************
*/

static void AppTaskLed(void *p_arg)
{
	OS_ERR err;

	CPU_BOOLEAN led_on = DEF_FALSE;

	while(DEF_TRUE){
		for(unsigned int i=1; i <= LED_COUNT; i++){
			if(led_on == DEF_TRUE)   BSP_LED_On(i);
			else  					 BSP_LED_Off(i);
		}
		if (led_on == DEF_FALSE) led_on = DEF_TRUE;
		else led_on = DEF_TRUE;
		OSTimeDlyHMSM(0u, 0u, 0u, 500u, OS_OPT_TIME_HMSM_NON_STRICT, &err);
	}
}

/*
*********************************************************************************************************
*                                          COMMAND PARSER
*********************************************************************************************************
*/

static void HandleCommand(char *cmd)
{
    (void)cmd;

}

/*
*********************************************************************************************************
*                                          GPIO SETUP
*********************************************************************************************************
*/

static void Setup_Gpio(void)
{
    GPIO_InitTypeDef gpio_init = {0};

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

    gpio_init.GPIO_Mode  = GPIO_Mode_OUT;
    gpio_init.GPIO_OType = GPIO_OType_PP;
    gpio_init.GPIO_Speed = GPIO_Speed_2MHz;
    gpio_init.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    gpio_init.GPIO_Pin   = GPIO_Pin_0 | GPIO_Pin_7 | GPIO_Pin_14;

    GPIO_Init(GPIOB, &gpio_init);
}

/*
*********************************************************************************************************
*                                          USART3 SETUP
*********************************************************************************************************
*/

static void Setup_Usart3(void)
{
    GPIO_InitTypeDef gpio_init = {0};
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

    usart_init.USART_BaudRate = 115200;
    usart_init.USART_WordLength = USART_WordLength_8b;
    usart_init.USART_StopBits = USART_StopBits_1;
    usart_init.USART_Parity = USART_Parity_No;
    usart_init.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    usart_init.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

    USART_Init(USART3, &usart_init);
    USART_Cmd(USART3, ENABLE);
}

/*
*********************************************************************************************************
*                                          USART OUTPUT HELPERS
*********************************************************************************************************
*/

static char UsartGetChar(void)
{
	if (USART_GetFlagStatus(USART3, USART_FLAG_RXNE) == SET){
		char ch= (char)(USART_ReceiveData(USART3) & 0xFFu);
		return ch;
	}
	return '\0';
}

static void UsartPutChar(char c)
{
    while (USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET) {}

    USART_SendData(USART3, (uint16_t)c);
}

static void UsartPrint(const char *s)
{
    while (*s != '\0') {
        UsartPutChar(*s++);
    }
}
