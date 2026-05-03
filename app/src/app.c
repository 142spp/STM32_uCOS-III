/*
*********************************************************************************************************
*                                              EXAMPLE CODE
*
*                             (c) Copyright 2013; Micrium, Inc.; Weston, FL
*
*                   All rights reserved.  Protected by international copyright laws.
*                   Knowledge of the source code may not be used to write a similar
*                   product.  This file may only be used in accordance with a license
*                   and should not be redistributed in any way.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                            EXAMPLE CODE
*
*                                       IAR Development Kits
*                                              on the
*
*                                    STM32F429II-SK KICKSTART KIT
*
* Filename      : app.c
* Version       : V1.00
* Programmer(s) : YS
*                 DC
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/

#include  <includes.h>
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_usart.h"
#include "stm32f4xx.h"
/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

#define  APP_TASK_EQ_0_ITERATION_NBR              16u
/*
*********************************************************************************************************
*                                            TYPES DEFINITIONS
*********************************************************************************************************
*/
typedef enum {
   TASK_SERIAL_RX,
   TASK_500MS,
   TASK_1000MS,
   TASK_2000MS,

   TASK_N
}task_e;
typedef struct
{
   CPU_CHAR* name;
   OS_TASK_PTR func;
   OS_PRIO prio;
   CPU_STK* pStack;
   OS_TCB* pTcb;
}task_t;

/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/
static  void  AppTaskStart          (void     *p_arg);
static  void  AppTaskCreate         (void);
static  void  AppObjCreate          (void);

static void AppTask_500ms(void *p_arg);
static void AppTask_1000ms(void *p_arg);
static void AppTask_2000ms(void *p_arg);
static void AppTask_SerialRx(void *p_arg);

static void Setup_Gpio(void);
static void Serial_Init(void);
static CPU_BOOLEAN Serial_ReadChar(char *ch);
static void Serial_WriteChar(char ch);
static void Serial_WriteString(const char *str);


/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/
/* ----------------- APPLICATION GLOBALS -------------- */
static  OS_TCB   AppTaskStartTCB;
static  CPU_STK  AppTaskStartStk[APP_CFG_TASK_START_STK_SIZE];

static  OS_TCB   Task_500ms_TCB;
static  OS_TCB   Task_1000ms_TCB;
static  OS_TCB   Task_2000ms_TCB;
static  OS_TCB   Task_SerialRx_TCB;

static  CPU_STK  Task_500ms_Stack[APP_CFG_TASK_START_STK_SIZE];
static  CPU_STK  Task_1000ms_Stack[APP_CFG_TASK_START_STK_SIZE];
static  CPU_STK  Task_2000ms_Stack[APP_CFG_TASK_START_STK_SIZE];
static  CPU_STK  Task_SerialRx_Stack[APP_CFG_TASK_START_STK_SIZE];
volatile int count=0;
task_t cyclic_tasks[TASK_N] = {
   {"Task_SerialRx", AppTask_SerialRx, 0, &Task_SerialRx_Stack[0], &Task_SerialRx_TCB},
   {"Task_500ms" , AppTask_500ms,  0, &Task_500ms_Stack[0] , &Task_500ms_TCB},
   {"Task_1000ms", AppTask_1000ms, 0, &Task_1000ms_Stack[0], &Task_1000ms_TCB},
   {"Task_2000ms", AppTask_2000ms, 0, &Task_2000ms_Stack[0], &Task_2000ms_TCB},
};
/* ------------ FLOATING POINT TEST TASK -------------- */
/*
*********************************************************************************************************
*                                                main()
*
* Description : This is the standard entry point for C code.  It is assumed that your code will call
*               main() once you have performed all necessary initialization.
*
* Arguments   : none
*
* Returns     : none
*********************************************************************************************************
*/

int main(void)
{
    OS_ERR  err;

    /* Basic Init */
    RCC_DeInit();
//    SystemCoreClockUpdate();
    Setup_Gpio();

    /* BSP Init */
    BSP_IntDisAll();                                            /* Disable all interrupts.                              */

    CPU_Init();                                                 /* Initialize the uC/CPU Services                       */
    Mem_Init();                                                 /* Initialize Memory Management Module                  */
    Math_Init();                                                /* Initialize Mathematical Module                       */


    /* OS Init */
    OSInit(&err);                                               /* Init uC/OS-III.                                      */

    OSTaskCreate((OS_TCB*)       	&AppTaskStartTCB,              /* Create the start task                                */
                 (CPU_CHAR*)   		"App Task Start",
                 (OS_TASK_PTR) 		AppTaskStart,
                 (void*)         	0u,
                 (OS_PRIO)			APP_CFG_TASK_START_PRIO,
                 (CPU_STK*)		&AppTaskStartStk[0u],
                 (CPU_STK_SIZE)	AppTaskStartStk[APP_CFG_TASK_START_STK_SIZE / 10u],
                 (CPU_STK_SIZE)	APP_CFG_TASK_START_STK_SIZE,
                 (OS_MSG_QTY)		0u,
                 (OS_TICK)		0u,
                 (void*)				0u,
                 (OS_OPT)				(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR*)			&err
				);

   OSStart(&err);   /* Start multitasking (i.e. give control to uC/OS-III). */

   (void)&err;

   return (0u);
}
/*
*********************************************************************************************************
*                                          STARTUP TASK
*
* Description : This is an example of a startup task.  As mentioned in the book's text, you MUST
*               initialize the ticker only once multitasking has started.
*
* Arguments   : p_arg   is the argument passed to 'AppTaskStart()' by 'OSTaskCreate()'.
*
* Returns     : none
*
* Notes       : 1) The first line of code is used to prevent a compiler warning because 'p_arg' is not
*                  used.  The compiler should not generate any code for this statement.
*********************************************************************************************************
*/
static  void  AppTaskStart (void *p_arg)
{
    OS_ERR  err;

   (void)p_arg;

    BSP_Init();                                                 /* Initialize BSP functions                             */
    BSP_Tick_Init();                                            /* Initialize Tick Services.                            */
    Serial_Init();                                              /* Initialize USART after BSP clock setup.              */

#if OS_CFG_STAT_TASK_EN > 0u
    OSStatTaskCPUUsageInit(&err);                               /* Compute CPU capacity with no task running            */
#endif

#ifdef CPU_CFG_INT_DIS_MEAS_EN
    CPU_IntDisMeasMaxCurReset();
#endif

   // BSP_LED_Off(0u);                                            /* Turn Off LEDs after initialization                   */

   APP_TRACE_DBG(("Creating Application Kernel Objects\n\r"));
   AppObjCreate();                                             /* Create Applicaiton kernel objects                    */

   APP_TRACE_DBG(("Creating Application Tasks\n\r"));
   AppTaskCreate();                                            /* Create Application tasks                             */
}

/*
*********************************************************************************************************
*                                          AppTask_500ms
*
* Description : Example of 500mS Task
*
* Arguments   : p_arg (unused)
*
* Returns     : none
*
* Note: Long period used to measure timing in person
*********************************************************************************************************
*/
static void AppTask_500ms(void *p_arg)
{
    OS_ERR  err;
    BSP_LED_On(1);
    while (DEF_TRUE) {                                          /* Task body, always written as an infinite loop.       */
       BSP_LED_Toggle(1);
        OSTimeDlyHMSM(0u, 0u, 0u, 500u,
                      OS_OPT_TIME_HMSM_STRICT,
                      &err);
       count++;

    }
}

/*
*********************************************************************************************************
*                                          AppTask_1000ms
*
* Description : Example of 1000mS Task
*
* Arguments   : p_arg (unused)
*
* Returns     : none
*
* Note: Long period used to measure timing in person
*********************************************************************************************************
*/
static void AppTask_1000ms(void *p_arg)
{
    OS_ERR  err;
    CPU_INT32U tx_count = 0u;
    char msg[64];

    BSP_LED_On(2);
    while (DEF_TRUE) {                                          /* Task body, always written as an infinite loop.       */
       BSP_LED_Toggle(2);
       sprintf(msg, "USART3 hello from uC/OS-III: %u\r\n", (unsigned int)tx_count++);
       Serial_WriteString(msg);

        OSTimeDlyHMSM(0u, 0u, 1u, 0u,
                      OS_OPT_TIME_HMSM_STRICT,
                      &err);

    }
}

/*
*********************************************************************************************************
*                                          AppTask_2000ms
*
* Description : Example of 2000mS Task
*
* Arguments   : p_arg (unused)
*
* Returns     : none
*
* Note: Long period used to measure timing in person
*********************************************************************************************************
*/
static void AppTask_2000ms(void *p_arg)
{
    OS_ERR  err;
    BSP_LED_On(3);
    while (DEF_TRUE) {                                          /* Task body, always written as an infinite loop.       */
       BSP_LED_Toggle(3);

        OSTimeDlyHMSM(0u, 0u, 2u, 0u,
                      OS_OPT_TIME_HMSM_STRICT,
                      &err);

    }
}

static void AppTask_SerialRx(void *p_arg)
{
   OS_ERR err;
   char ch;

   (void)p_arg;

   Serial_WriteString("Type characters to echo over USART3\r\n");

   while (DEF_TRUE) {
      while (Serial_ReadChar(&ch) == DEF_TRUE) {
         Serial_WriteString("rx: ");
         Serial_WriteChar(ch);
         Serial_WriteString("\r\n");
      }

      OSTimeDlyHMSM(0u, 0u, 0u, 10u,
                    OS_OPT_TIME_HMSM_STRICT,
                    &err);
   }
}

/*
*********************************************************************************************************
*                                          AppTaskCreate()
*
* Description : Create application tasks.
*
* Argument(s) : none
*
* Return(s)   : none
*
* Caller(s)   : AppTaskStart()
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  AppTaskCreate (void)
{
   OS_ERR  err;

   u8_t idx = 0;
   task_t* pTask_Cfg;
   for(idx = 0; idx < TASK_N; idx++)
   {
      pTask_Cfg = &cyclic_tasks[idx];

      OSTaskCreate(
            (OS_TCB *) 		pTask_Cfg->pTcb,
            (CPU_CHAR *) 	pTask_Cfg->name,
            (OS_TASK_PTR) 	pTask_Cfg->func,
            (void *)			0u,
            (OS_PRIO) 		pTask_Cfg->prio,
            (CPU_STK *)	pTask_Cfg->pStack,
            (CPU_STK) 	pTask_Cfg->pStack[APP_CFG_TASK_START_STK_SIZE / 10u],
			(CPU_STK)		APP_CFG_TASK_START_STK_SIZE,
            (OS_MSG_QTY)	0u,
            (OS_TICK)	0u,
            (void *)			0u,
            (OS_OPT)			(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
            (OS_ERR *)		&err
      );
   }
}


/*
*********************************************************************************************************
*                                          AppObjCreate()
*
* Description : Create application kernel objects tasks.
*
* Argument(s) : none
*
* Return(s)   : none
*
* Caller(s)   : AppTaskStart()
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  AppObjCreate (void)
{

}

/*
*********************************************************************************************************
*                                          Setup_Gpio()
*
* Description : Configure LED GPIOs directly
*
* Argument(s) : none
*
* Return(s)   : none
*
* Caller(s)   : AppTaskStart()
*
* Note(s)     :
*              LED1 PB0
*              LED2 PB7
*              LED3 PB14
*
*********************************************************************************************************
*/
static void Setup_Gpio(void)
{
   GPIO_InitTypeDef led_init = {0};

   RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
   RCC_AHB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

   led_init.GPIO_Mode   = GPIO_Mode_OUT;
   led_init.GPIO_OType  = GPIO_OType_PP;
   led_init.GPIO_Speed  = GPIO_Speed_2MHz;
   led_init.GPIO_PuPd   = GPIO_PuPd_NOPULL;
   led_init.GPIO_Pin    = GPIO_Pin_0 | GPIO_Pin_7 | GPIO_Pin_14;

   GPIO_Init(GPIOB, &led_init);
}

/*
*********************************************************************************************************
*                                          Serial_Init()
*
* Description : Configure USART3 for the NUCLEO ST-LINK virtual COM port.
*
* Note(s)     : NUCLEO-F429ZI VCP uses USART3 PD8(TX) / PD9(RX).
*********************************************************************************************************
*/
static void Serial_Init(void)
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

static CPU_BOOLEAN Serial_ReadChar(char *ch)
{
   if (USART_GetFlagStatus(USART3, USART_FLAG_RXNE) == RESET) {
      return DEF_FALSE;
   }

   *ch = (char)(USART_ReceiveData(USART3) & 0xFFu);
   return DEF_TRUE;
}

static void Serial_WriteChar(char ch)
{
   while (USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET) {
   }

   USART_SendData(USART3, (uint16_t)ch);
}

static void Serial_WriteString(const char *str)
{
   while (*str != '\0') {
      Serial_WriteChar(*str++);
   }
}

