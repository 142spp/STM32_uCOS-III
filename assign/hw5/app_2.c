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

#include <ctype.h>
#include <includes.h>
#include "bsp.h"
#include "cpu.h"
#include "lib_str.h"
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
#define CMD_BUF_SIZE			32u
#define LED_COUNT				 3u
#define APP_TASK_USART_PRIO		10u
#define APP_TASK_LED_PRIO		11u

/*
*********************************************************************************************************
*                                            LOCAL TYPES
*********************************************************************************************************
*/

typedef enum {
	LED_ON, 
	LED_OFF, 
	LED_BLINK
} ledcmd;

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

static char UsartGetChar(void);
static void UsartPutChar(char c);
static void UsartPrint(const char *s);
static void UsartPrintLedMsg(CPU_INT08U led, ledcmd mode, CPU_INT08U period);

static void HandleCommand(char *cmd);
static int  min(int a, int b);

/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/

static OS_TCB  	AppTaskStartTCB;
static CPU_STK 	AppTaskStartStk[APP_CFG_TASK_START_STK_SIZE];

static OS_TCB  	UsartTaskTCB;
static CPU_STK 	UsartTaskStk[APP_TASK_STK_SIZE];

static OS_TCB  	LedTaskTCB;
static CPU_STK 	LedTaskStk[APP_TASK_STK_SIZE];


static ledcmd 	LedMode[LED_COUNT+1];
static int 		LedPeriod[LED_COUNT+1];
static char 	CmdBuffer[CMD_BUF_SIZE];
static int		CmdLen;

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

    Setup_Usart3();

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
	OS_ERR err;
	char ch;
	CPU_SR_ALLOC();

	while(DEF_TRUE){	
		ch = UsartGetChar();
		if( ch != '\0' ){
			if (ch == '\r' || ch == '\n') {
				OS_CRITICAL_ENTER();
				CmdLen = 0;
				OS_CRITICAL_EXIT();
				UsartPrint("\r\n");
				continue;
			}				
			OS_CRITICAL_ENTER();
			if (CmdLen >= (int)(CMD_BUF_SIZE - 1)) {
				CmdLen = 0;
				OS_CRITICAL_EXIT();
				UsartPrint("\r\nWrong Command\r\n");
				continue;
			}
			CmdBuffer[CmdLen++] = ch;
			CmdBuffer[CmdLen] = '\0';
			OS_CRITICAL_EXIT();
			
			UsartPutChar(ch); 
			HandleCommand(CmdBuffer);
			
		} 
		OSTimeDlyHMSM(0u, 0u, 0u, 10u, \
			OS_OPT_TIME_HMSM_NON_STRICT, &err);
	}
}

/*
*********************************************************************************************************
*                                          LED CONTROL TASK
*********************************************************************************************************
*/

static void AppTaskLed(void *p_arg)
{
	OS_ERR err;
	
	CPU_INT08U 	led;
	CPU_BOOLEAN ledstats[LED_COUNT+1] = {0u,0u,0u,0u};
	CPU_INT08U 	ledseconds[LED_COUNT+1] = {0u,0u,0u,0u};
	
	ledcmd mode;
	int period;
	
	CPU_SR_ALLOC();

	while(DEF_TRUE){
		for(led=1u; led <= LED_COUNT; led++){
			OS_CRITICAL_ENTER();
			mode = LedMode[led];
			period = LedPeriod[led];
			OS_CRITICAL_EXIT();
			if(mode == LED_ON || (mode == LED_BLINK && ledstats[led] == 1u)){
				BSP_LED_On(led);
				ledstats[led] = DEF_TRUE;
			}else{
				BSP_LED_Off(led);
				ledstats[led] = DEF_FALSE;
			}
			if(mode == LED_BLINK){
				ledseconds[led]++;
				if(ledseconds[led] == period){
					ledstats[led] ^= DEF_TRUE;
					ledseconds[led] = 0u;
				}
			}
		}
		OSTimeDlyHMSM(0u, 0u, 0u, 1000u, \
			OS_OPT_TIME_HMSM_NON_STRICT, &err);
	}
}

/*
*********************************************************************************************************
*                                          COMMAND PARSER
*********************************************************************************************************
*/

static void HandleCommand(char *cmd){

	CPU_SR_ALLOC();
	OS_CRITICAL_ENTER();
	CPU_INT08U cmdlen = CmdLen;
	OS_CRITICAL_EXIT();
	
	CPU_BOOLEAN wrongflag = DEF_FALSE;
	CPU_INT08U led;
	CPU_INT08U lednum;

	while(DEF_TRUE){
		if (Str_Cmp_N(cmd, "reset", min(cmdlen,5)) == 0 ){
			if(cmdlen < 5 ) break;
			OS_CRITICAL_ENTER();
			for ( led = 1u; led <= LED_COUNT; led++ ) {
				LedMode[led] = LED_OFF;
			}
			OS_CRITICAL_EXIT();
			for ( led = 1u; led <= LED_COUNT; led++ ) {
				BSP_LED_Off(led);
				UsartPrintLedMsg( led, LED_OFF, 0);
			}
			cmdlen = 0;
		}
		else if (Str_Cmp_N(cmd, "led", min(cmdlen,3)) == 0){
			if( cmdlen < 4 ) break;
			if( *(cmd+3) < '1' || *(cmd+3) > '3' ) { wrongflag=DEF_TRUE; break; }
			lednum = *(cmd+3)-'0';
			if( Str_Cmp_N(cmd+4,"on",cmdlen-4) == 0){
				if(cmdlen<6) break;
				OS_CRITICAL_ENTER();
				LedMode[lednum] = LED_ON;
				OS_CRITICAL_EXIT();
				BSP_LED_On(lednum);
				UsartPrintLedMsg( lednum, LED_ON, 0);
				cmdlen = 0;
			}
			else if(Str_Cmp_N(cmd+4,"off",cmdlen-4) == 0){
				if(cmdlen < 7) break;
				OS_CRITICAL_ENTER();
				LedMode[lednum] = LED_OFF;
				OS_CRITICAL_EXIT();
				BSP_LED_Off(lednum);
				UsartPrintLedMsg( lednum, LED_OFF, 0);
				cmdlen = 0;
			}
			else if(Str_Cmp_N(cmd+4, "blink", min(cmdlen-4,5)) == 0){
				if( cmdlen < 10 ) break;
				if( *(cmd+9) < '1' || *(cmd+9) > '9') {wrongflag = DEF_TRUE; break;}
				OS_CRITICAL_ENTER();
				LedMode[lednum] = LED_BLINK;
				LedPeriod[lednum] = *(cmd+9) - '0';
				OS_CRITICAL_EXIT();
				UsartPrintLedMsg( lednum, LED_BLINK, *(cmd+9)-'0');
				cmdlen = 0;
			}
			else { wrongflag=DEF_TRUE; break; }
		}
		else { wrongflag=DEF_TRUE; break; }
	}
	if(wrongflag == DEF_TRUE){
		UsartPrint("\r\nWrong Command\r\n");
		cmdlen = 0;
	}
	OS_CRITICAL_ENTER();
	CmdLen = cmdlen;
	OS_CRITICAL_EXIT();
}

static int min(int a, int b){
	if(a>b) return b;
	else return a;
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

static void UsartPrintLedMsg(CPU_INT08U led, ledcmd mode, CPU_INT08U period){
	UsartPrint("\r\nLED ");
	UsartPutChar('0' + led);
	UsartPrint(" ");

	if(mode == LED_ON) UsartPrint("On");
	else if(mode == LED_OFF) UsartPrint("Off");
	else if(mode == LED_BLINK) UsartPrint("Blink");

	if (period > 0u) {
		UsartPrint(" ");
		UsartPutChar('0' + period);
	}

	UsartPrint("\r\n");
}
