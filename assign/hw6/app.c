/*
*********************************************************************************************************
* HW #6 Starter app.c
*
* uC/OS-III task communication assignment starter for STM32F429II-SK / NUCLEO-F429ZI style examples.
*
 * This file is a scaffold, not a complete answer.
 * Core implementation parts are intentionally hidden as TODO comments.
*********************************************************************************************************
*/

#include  <includes.h>
#include  "stm32f4xx.h"
#include  "stm32f4xx_rcc.h"
#include  "stm32f4xx_gpio.h"
#include  "stm32f4xx_exti.h"
#include  "stm32f4xx_syscfg.h"
#include  "misc.h"

/*
*********************************************************************************************************
* LOCAL DEFINES
*********************************************************************************************************
*/

#define  APP_CFG_INPUT_TASK_PRIO                 4u
#define  APP_CFG_USART_TASK_PRIO                 5u
#define  APP_CFG_OUTPUT_TASK_PRIO                6u
#define  APP_CFG_MONITOR_TASK_PRIO               7u

#define  APP_CFG_TASK_STK_SIZE                   APP_CFG_TASK_START_STK_SIZE

#define  APP_EVENT_Q_SIZE                        10u
#define  APP_CMD_LINE_LEN                        32u

#define  APP_LED_1                               1u
#define  APP_LED_2                               2u
#define  APP_LED_3                               3u

#define  APP_BUTTON_DEBOUNCE_MS                  50u

/*
*********************************************************************************************************
* TYPES
*********************************************************************************************************
*/

typedef enum {
    APP_EVENT_NONE = 0,
    APP_EVENT_BUTTON_PRESS,
    APP_EVENT_CMD_MODE_ROLLING,
    APP_EVENT_CMD_MODE_BLINK,
    APP_EVENT_CMD_MODE_OFF,
    APP_EVENT_CMD_TARGET,
    APP_EVENT_CMD_STATUS,
    APP_EVENT_CMD_RESET,
    APP_EVENT_CMD_HELP
} APP_EVENT_TYPE;

typedef enum {
    APP_LED_MODE_OFF = 0,
    APP_LED_MODE_ROLLING,
    APP_LED_MODE_BLINK
} APP_LED_MODE;

typedef struct {
    APP_EVENT_TYPE  Type;
    CPU_INT08U      Target;
    CPU_INT32U      Value;
} APP_EVENT;

/*
*********************************************************************************************************
* FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void  AppTaskStart        (void *p_arg);
static  void  AppTaskInput        (void *p_arg);
static  void  AppTaskUsart        (void *p_arg);
static  void  AppTaskOutput       (void *p_arg);
static  void  AppTaskMonitor      (void *p_arg);

static  void  AppTaskCreate       (void);
static  void  AppObjCreate        (void);

static  void  AppGpioInit         (void);
static  void  AppButtonExtiInit   (void);
static  void  AppButtonISR        (void);

static  void  AppEventPost        (APP_EVENT_TYPE type,
                                   CPU_INT08U     target,
                                   CPU_INT32U     value);
static  void  AppCmdParse         (CPU_CHAR      *line);
static  void  AppPrintHelp        (void);
static  void  AppPrintStatus      (void);
static  void  AppTrace            (const CPU_CHAR *msg);
static  CPU_BOOLEAN AppUsartTryGetLine(CPU_CHAR *buf,
                                       CPU_INT16U buf_len);
static  CPU_BOOLEAN AppStrEq      (const CPU_CHAR *a,
                                   const CPU_CHAR *b);

/*
*********************************************************************************************************
* GLOBAL VARIABLES
*********************************************************************************************************
*/

static  OS_TCB   AppTaskStartTCB;
static  CPU_STK  AppTaskStartStk[APP_CFG_TASK_START_STK_SIZE];

static  OS_TCB   AppTaskInputTCB;
static  OS_TCB   AppTaskUsartTCB;
static  OS_TCB   AppTaskOutputTCB;
static  OS_TCB   AppTaskMonitorTCB;

static  CPU_STK  AppTaskInputStk[APP_CFG_TASK_STK_SIZE];
static  CPU_STK  AppTaskUsartStk[APP_CFG_TASK_STK_SIZE];
static  CPU_STK  AppTaskOutputStk[APP_CFG_TASK_STK_SIZE];
static  CPU_STK  AppTaskMonitorStk[APP_CFG_TASK_STK_SIZE];

static  OS_SEM   AppButtonSem;
static  OS_Q     AppEventQ;

static  APP_EVENT  AppEventPool[APP_EVENT_Q_SIZE];
static  CPU_INT08U AppEventWrIx;

static  APP_LED_MODE AppLedMode;
static  CPU_INT08U   AppTargetLed;
static  CPU_INT08U   AppRollingLed;
static  CPU_INT32U   AppButtonCount;
static  CPU_INT32U   AppCommandCount;

/*
*********************************************************************************************************
* main()
*********************************************************************************************************
*/

int main(void)
{
    OS_ERR  err;

    RCC_DeInit();
    AppGpioInit();

    BSP_IntDisAll();

    CPU_Init();
    Mem_Init();
    Math_Init();

    OSInit(&err);

    OSTaskCreate((OS_TCB       *)&AppTaskStartTCB,
                 (CPU_CHAR     *)"App Task Start",
                 (OS_TASK_PTR   ) AppTaskStart,
                 (void         *) 0u,
                 (OS_PRIO       ) APP_CFG_TASK_START_PRIO,
                 (CPU_STK      *)&AppTaskStartStk[0u],
                 (CPU_STK_SIZE  ) APP_CFG_TASK_START_STK_SIZE / 10u,
                 (CPU_STK_SIZE  ) APP_CFG_TASK_START_STK_SIZE,
                 (OS_MSG_QTY    ) 0u,
                 (OS_TICK       ) 0u,
                 (void         *) 0u,
                 (OS_OPT        )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR       *)&err);

    OSStart(&err);

    (void)&err;

    return (0u);
}

/*
*********************************************************************************************************
* START TASK
*********************************************************************************************************
*/

static void AppTaskStart(void *p_arg)
{
    OS_ERR err;

    (void)p_arg;

    BSP_Init();
    BSP_Tick_Init();
    AppButtonExtiInit();

#if OS_CFG_STAT_TASK_EN > 0u
    OSStatTaskCPUUsageInit(&err);
#endif

#ifdef CPU_CFG_INT_DIS_MEAS_EN
    CPU_IntDisMeasMaxCurReset();
#endif

    AppTrace("\r\n[BOOT] HW6 starter\r\n");

    AppObjCreate();
    AppTaskCreate();

    while (DEF_TRUE) {
        OSTimeDlyHMSM(0u, 0u, 1u, 0u,
                      OS_OPT_TIME_HMSM_STRICT,
                      &err);
    }
}

/*
*********************************************************************************************************
* INPUT TASK
*
* The ISR only posts AppButtonSem. This task performs debounce and forwards a button event to AppEventQ.
*********************************************************************************************************
*/

static void AppTaskInput(void *p_arg)
{
    OS_ERR err;
    CPU_TS ts;

    (void)p_arg;

    while (DEF_TRUE) {
        /*
         * TODO:
         * 1. Wait for the semaphore posted by AppButtonISR().
         * 2. Apply debounce delay.
         * 3. Increase AppButtonCount.
         * 4. Forward a button event to AppEventQ using AppEventPost().
         * 5. Print a short USART log.
         */
        (void)&ts;
        (void)&err;
        OSTimeDlyHMSM(0u, 0u, 0u, 100u,
                      OS_OPT_TIME_HMSM_STRICT,
                      &err);
    }
}

/*
*********************************************************************************************************
* USART TASK
*
* Replace AppUsartTryGetLine() with the USART receive code from HW #5.
*********************************************************************************************************
*/

static void AppTaskUsart(void *p_arg)
{
    OS_ERR err;
    CPU_CHAR line[APP_CMD_LINE_LEN];

    (void)p_arg;

    AppPrintHelp();

    while (DEF_TRUE) {
        /*
         * TODO:
         * If one USART command line has arrived:
         * - increase AppCommandCount
         * - parse the command
         * - send an event to OutputTask
         */
        if (AppUsartTryGetLine(line, sizeof(line)) == DEF_TRUE) {
            AppCmdParse(line);
        }

        OSTimeDlyHMSM(0u, 0u, 0u, 20u,
                      OS_OPT_TIME_HMSM_STRICT,
                      &err);
    }
}

/*
*********************************************************************************************************
* OUTPUT TASK
*
* Receives button/USART events and controls LEDs.
*********************************************************************************************************
*/

static void AppTaskOutput(void *p_arg)
{
    OS_ERR       err;
    OS_MSG_SIZE  msg_size;
    CPU_TS       ts;
    APP_EVENT   *p_event = (APP_EVENT *)0;

    (void)p_arg;

    AppLedMode    = APP_LED_MODE_OFF;
    AppTargetLed  = APP_LED_2;
    AppRollingLed = APP_LED_1;
    BSP_LED_Off(0u);

    while (DEF_TRUE) {
        /*
         * TODO:
         * 1. Wait for one APP_EVENT from AppEventQ.
         * 2. Handle button events and USART command events.
         * 3. Update AppLedMode, AppTargetLed, and AppRollingLed.
         * 4. Print the result over USART.
         */
        (void)p_event;
        (void)&msg_size;
        (void)&ts;

        /*
         * TODO:
         * Implement LED behavior for each mode.
         *
         * Examples:
         * - APP_LED_MODE_OFF: all LEDs off
         * - APP_LED_MODE_BLINK: blink selected target LED
         * - APP_LED_MODE_ROLLING: LED1 -> LED2 -> LED3
        */
        BSP_LED_Off(0u);

        OSTimeDlyHMSM(0u, 0u, 0u, 100u,
                      OS_OPT_TIME_HMSM_STRICT,
                      &err);
    }
}

/*
*********************************************************************************************************
* MONITOR TASK
*********************************************************************************************************
*/

static void AppTaskMonitor(void *p_arg)
{
    OS_ERR err;

    (void)p_arg;

    while (DEF_TRUE) {
        AppPrintStatus();
        OSTimeDlyHMSM(0u, 0u, 2u, 0u,
                      OS_OPT_TIME_HMSM_STRICT,
                      &err);
    }
}

/*
*********************************************************************************************************
* KERNEL OBJECTS
*********************************************************************************************************
*/

static void AppObjCreate(void)
{
    OS_ERR err;

    /*
     * TODO:
     * Create the semaphore used by the button ISR.
     *
     * Hint:
     * - Initial count should be 0 because no button event has occurred yet.
     */
    (void)&AppButtonSem;

    /*
     * TODO:
     * Create the application event queue used for task-to-task communication.
     */
    (void)&AppEventQ;
    (void)&err;
}

/*
*********************************************************************************************************
* TASK CREATE
*********************************************************************************************************
*/

static void AppTaskCreate(void)
{
    OS_ERR err;

    OSTaskCreate(&AppTaskInputTCB,
                 "Input Task",
                 AppTaskInput,
                 (void *)0u,
                 APP_CFG_INPUT_TASK_PRIO,
                 &AppTaskInputStk[0u],
                 APP_CFG_TASK_STK_SIZE / 10u,
                 APP_CFG_TASK_STK_SIZE,
                 0u,
                 0u,
                 (void *)0u,
                 OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR,
                 &err);

    OSTaskCreate(&AppTaskUsartTCB,
                 "USART Task",
                 AppTaskUsart,
                 (void *)0u,
                 APP_CFG_USART_TASK_PRIO,
                 &AppTaskUsartStk[0u],
                 APP_CFG_TASK_STK_SIZE / 10u,
                 APP_CFG_TASK_STK_SIZE,
                 0u,
                 0u,
                 (void *)0u,
                 OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR,
                 &err);

    OSTaskCreate(&AppTaskOutputTCB,
                 "Output Task",
                 AppTaskOutput,
                 (void *)0u,
                 APP_CFG_OUTPUT_TASK_PRIO,
                 &AppTaskOutputStk[0u],
                 APP_CFG_TASK_STK_SIZE / 10u,
                 APP_CFG_TASK_STK_SIZE,
                 0u,
                 0u,
                 (void *)0u,
                 OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR,
                 &err);

    OSTaskCreate(&AppTaskMonitorTCB,
                 "Monitor Task",
                 AppTaskMonitor,
                 (void *)0u,
                 APP_CFG_MONITOR_TASK_PRIO,
                 &AppTaskMonitorStk[0u],
                 APP_CFG_TASK_STK_SIZE / 10u,
                 APP_CFG_TASK_STK_SIZE,
                 0u,
                 0u,
                 (void *)0u,
                 OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR,
                 &err);
}

/*
*********************************************************************************************************
* GPIO / EXTI
*********************************************************************************************************
*/

static void AppGpioInit(void)
{
    GPIO_InitTypeDef led_init;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

    led_init.GPIO_Mode  = GPIO_Mode_OUT;
    led_init.GPIO_OType = GPIO_OType_PP;
    led_init.GPIO_Speed = GPIO_Speed_2MHz;
    led_init.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    led_init.GPIO_Pin   = GPIO_Pin_0 | GPIO_Pin_7 | GPIO_Pin_14;

    GPIO_Init(GPIOB, &led_init);
}

static void AppButtonExtiInit(void)
{
    GPIO_InitTypeDef gpio_init;
    EXTI_InitTypeDef exti_init;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

    gpio_init.GPIO_Pin   = GPIO_Pin_13;
    gpio_init.GPIO_Mode  = GPIO_Mode_IN;
    gpio_init.GPIO_OType = GPIO_OType_PP;
    gpio_init.GPIO_Speed = GPIO_Speed_2MHz;
    gpio_init.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOC, &gpio_init);

    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOC,
                          EXTI_PinSource13);

    EXTI_ClearITPendingBit(EXTI_Line13);

    exti_init.EXTI_Line    = EXTI_Line13;
    exti_init.EXTI_Mode    = EXTI_Mode_Interrupt;
    exti_init.EXTI_Trigger = EXTI_Trigger_Falling;
    exti_init.EXTI_LineCmd = ENABLE;
    EXTI_Init(&exti_init);

    BSP_IntVectSet(BSP_INT_ID_EXTI15_10, AppButtonISR);
    BSP_IntPrioSet(BSP_INT_ID_EXTI15_10, 5u);
    BSP_IntEn(BSP_INT_ID_EXTI15_10);
}

static void AppButtonISR(void)
{
    OS_ERR err;

    /*
     * TODO:
     * 1. Check whether EXTI line 13 caused this interrupt.
     * 2. Clear the EXTI pending bit.
     * 3. Notify InputTask using the semaphore.
     *
     * Do not call OSSemPend(), OSQPend(), OSTimeDlyHMSM(), or printf() here.
     */
    (void)&err;
}

/*
*********************************************************************************************************
* EVENT / COMMAND HELPERS
*********************************************************************************************************
*/

static void AppEventPost(APP_EVENT_TYPE type,
                         CPU_INT08U     target,
                         CPU_INT32U     value)
{
    APP_EVENT *p_event = (APP_EVENT *)0;
    OS_ERR     err;
    /*
     * TODO:
     * Select one event object from AppEventPool, fill Type/Target/Value,
     * then post its pointer to AppEventQ.
     *
     * Important:
     * uC/OS-III queues pass a pointer. Do not post the address of a local
     * variable that disappears after this function returns.
     */
    (void)p_event;
    (void)&err;
    (void)&AppEventPool[0];
    (void)&AppEventWrIx;
    (void)&AppEventQ;
    (void)type;
    (void)target;
    (void)value;
}

static void AppCmdParse(CPU_CHAR *line)
{
    /*
     * TODO:
     * Parse USART commands and convert them into APP_EVENT messages.
     *
     * Required commands:
     * - help
     * - status
     * - mode rolling
     * - mode blink
     * - mode off
     * - target1 / target2 / target3
     * - reset
     */
    (void)line;
}

static void AppPrintHelp(void)
{
    AppTrace("[HELP] help | status | mode rolling | mode blink | mode off | target1 | target2 | target3 | reset\r\n");
}

static void AppPrintStatus(void)
{
    APP_TRACE_DBG(("[MONITOR] mode=%d target=LED%d buttons=%lu cmds=%lu\r\n",
                   (int)AppLedMode,
                   (int)AppTargetLed,
                   (unsigned long)AppButtonCount,
                   (unsigned long)AppCommandCount));
}

/*
*********************************************************************************************************
* USART / STRING HELPERS
*********************************************************************************************************
*/

static void AppTrace(const CPU_CHAR *msg)
{
    APP_TRACE_DBG(("%s", msg));
}

static CPU_BOOLEAN AppUsartTryGetLine(CPU_CHAR *buf, CPU_INT16U buf_len)
{
    (void)buf;
    (void)buf_len;

    /*
     * TODO:
     * Replace this function with USART receive code from HW #5.
     *
     * Expected behavior:
     * - return DEF_TRUE when one command line is received.
     * - copy a NUL-terminated command string into buf.
     * - remove trailing '\r' or '\n'.
     *
     * Example command strings:
     *   "help"
     *   "status"
     *   "mode rolling"
     *   "mode blink"
     *   "mode off"
     *   "target2"
     *   "reset"
     */
    return DEF_FALSE;
}

static CPU_BOOLEAN AppStrEq(const CPU_CHAR *a, const CPU_CHAR *b)
{
    while ((*a != (CPU_CHAR)0) && (*b != (CPU_CHAR)0)) {
        if (*a != *b) {
            return DEF_FALSE;
        }
        a++;
        b++;
    }

    if ((*a == (CPU_CHAR)0) && (*b == (CPU_CHAR)0)) {
        return DEF_TRUE;
    }

    return DEF_FALSE;
}
