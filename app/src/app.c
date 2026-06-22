/*
*********************************************************************************************************
* HW #6 app.c
*
* uC/OS-III task communication assignment answer.
* Theme: rolling LED stop game using button interrupt, semaphore, queue, and USART monitor.
*********************************************************************************************************
*/

#include  <includes.h>
#include  "stm32f4xx.h"
#include  "stm32f4xx_rcc.h"
#include  "stm32f4xx_gpio.h"
#include  "stm32f4xx_exti.h"
#include  "stm32f4xx_syscfg.h"
#include  "stm32f4xx_usart.h"
#include  "misc.h"

#include  "app_parking.h"
#include  "task_button.h"

/*
*********************************************************************************************************
* APP SELECT
*   1: Smart Parking 텀프로젝트   0: HW#6 LED 게임
*********************************************************************************************************
*/

#define  APP_USE_PARKING                         1u

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
#define  APP_OUTPUT_POLL_MS                      20u
#define  APP_LED_UPDATE_MS                       300u

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
    APP_LED_MODE_BLINK,
    APP_LED_MODE_STOP
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
static  void  AppUsart3Init       (void);
static  void  AppButtonExtiInit   (void);
static  void  AppButtonISR        (void);

static  void  AppEventPost        (APP_EVENT_TYPE type,
                                   CPU_INT08U     target,
                                   CPU_INT32U     value);
static  void  AppCmdParse         (CPU_CHAR      *line);
static  void  AppPrintHelp        (void);
static  void  AppPrintStatus      (void);
static  void  AppTrace            (const CPU_CHAR *msg);
static  void  AppTraceChar        (CPU_CHAR ch);
static  void  AppTraceUInt        (CPU_INT32U value);
static  CPU_BOOLEAN AppUsartTryGetChar(CPU_CHAR *ch);
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
static  CPU_INT08U   AppVisibleLed;
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
    AppUsart3Init();

#if OS_CFG_STAT_TASK_EN > 0u
    OSStatTaskCPUUsageInit(&err);
#endif

#ifdef CPU_CFG_INT_DIS_MEAS_EN
    CPU_IntDisMeasMaxCurReset();
#endif

#if APP_USE_PARKING
    AppTrace("\r\n[BOOT] Smart Parking\r\n");

    ParkingApp_Init();          /* IPC + 전체 task 생성 (ButtonTask가 ButtonSem 생성) */
    AppButtonExtiInit();        /* 버튼 EXTI 하드웨어는 그대로 재사용 */
#else
    AppTrace("\r\n[BOOT] HW6 button ISR + task communication\r\n");

    AppObjCreate();
    AppButtonExtiInit();
    AppTaskCreate();
#endif

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
        OSSemPend(&AppButtonSem,
                  0u,
                  OS_OPT_PEND_BLOCKING,
                  &ts,
                  &err);

        if (err != OS_ERR_NONE) {
            continue;
        }

        OSTimeDlyHMSM(0u, 0u, 0u, APP_BUTTON_DEBOUNCE_MS,
                      OS_OPT_TIME_HMSM_STRICT,
                      &err);

        AppButtonCount++;
        AppTrace("[INPUT] button interrupt accepted\r\n");
        AppEventPost(APP_EVENT_BUTTON_PRESS, 0u, AppButtonCount);
    }
}

/*
*********************************************************************************************************
* USART TASK
*********************************************************************************************************
*/

static void AppTaskUsart(void *p_arg)
{
    OS_ERR err;
    CPU_CHAR line[APP_CMD_LINE_LEN];

    (void)p_arg;

    AppPrintHelp();

    while (DEF_TRUE) {
        if (AppUsartTryGetLine(line, sizeof(line)) == DEF_TRUE) {
            AppCommandCount++;
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
    CPU_BOOLEAN  led_on = DEF_FALSE;
    CPU_BOOLEAN  update_led = DEF_FALSE;
    CPU_INT08U   stopped_led;
    CPU_INT16U   led_elapsed_ms = APP_LED_UPDATE_MS;

    (void)p_arg;

    AppLedMode    = APP_LED_MODE_ROLLING;
    AppTargetLed  = APP_LED_2;
    AppRollingLed = APP_LED_1;
    AppVisibleLed = APP_LED_1;
    BSP_LED_Off(0u);

    while (DEF_TRUE) {
        update_led = DEF_FALSE;

        p_event = (APP_EVENT *)OSQPend(&AppEventQ,
                                       0u,
                                       OS_OPT_PEND_NON_BLOCKING,
                                       &msg_size,
                                       &ts,
                                       &err);

        if ((err == OS_ERR_NONE) && (p_event != (APP_EVENT *)0)) {
            switch (p_event->Type) {
                case APP_EVENT_BUTTON_PRESS:
                    stopped_led = AppVisibleLed;
                    AppTrace("[OUTPUT] button stop LED");
                    AppTraceUInt(stopped_led);
                    AppTrace(" target LED");
                    AppTraceUInt(AppTargetLed);

                    if (stopped_led == AppTargetLed) {
                        AppTrace(" HIT\r\n");
                        led_on = DEF_TRUE;
                        AppLedMode = APP_LED_MODE_BLINK;
                    } else {
                        AppTrace(" MISS\r\n");
                        AppLedMode = APP_LED_MODE_STOP;
                    }
                    update_led = DEF_TRUE;
                    break;

                case APP_EVENT_CMD_MODE_ROLLING:
                    AppLedMode = APP_LED_MODE_ROLLING;
                    led_elapsed_ms = APP_LED_UPDATE_MS;
                    update_led = DEF_TRUE;
                    AppTrace("[OUTPUT] mode rolling\r\n");
                    break;

                case APP_EVENT_CMD_MODE_BLINK:
                    led_on = DEF_TRUE;
                    AppLedMode = APP_LED_MODE_BLINK;
                    update_led = DEF_TRUE;
                    AppTrace("[OUTPUT] mode blink\r\n");
                    break;

                case APP_EVENT_CMD_MODE_OFF:
                    AppLedMode = APP_LED_MODE_OFF;
                    update_led = DEF_TRUE;
                    AppTrace("[OUTPUT] mode off\r\n");
                    break;

                case APP_EVENT_CMD_TARGET:
                    if ((p_event->Target >= APP_LED_1) &&
                        (p_event->Target <= APP_LED_3)) {
                        AppTargetLed = p_event->Target;
                        AppTrace("[OUTPUT] target LED");
                        AppTraceUInt(AppTargetLed);
                        AppTrace("\r\n");
                        update_led = DEF_TRUE;
                    }
                    break;

                case APP_EVENT_CMD_STATUS:
                    AppPrintStatus();
                    break;

                case APP_EVENT_CMD_RESET:
                    AppLedMode    = APP_LED_MODE_ROLLING;
                    AppTargetLed  = APP_LED_2;
                    AppRollingLed = APP_LED_1;
                    AppVisibleLed = APP_LED_1;
                    led_on        = DEF_FALSE;
                    led_elapsed_ms = APP_LED_UPDATE_MS;
                    update_led    = DEF_TRUE;
                    BSP_LED_Off(0u);
                    AppTrace("[OUTPUT] reset\r\n");
                    break;

                case APP_EVENT_CMD_HELP:
                    AppPrintHelp();
                    break;

                default:
                    break;
            }
        }

        if (led_elapsed_ms >= APP_LED_UPDATE_MS) {
            led_elapsed_ms = 0u;
            update_led = DEF_TRUE;
        }

        if (update_led == DEF_TRUE) {
            if (AppLedMode == APP_LED_MODE_OFF) {
                BSP_LED_Off(0u);
            } else if (AppLedMode == APP_LED_MODE_BLINK) {
                BSP_LED_Off(0u);
                if (led_on == DEF_TRUE) {
                    BSP_LED_On(AppTargetLed);
                    AppVisibleLed = AppTargetLed;
                }
                led_on = (led_on == DEF_TRUE) ? DEF_FALSE : DEF_TRUE;
            } else if (AppLedMode == APP_LED_MODE_STOP) {
                BSP_LED_Off(0u);
                BSP_LED_On(AppVisibleLed);
            } else {
                BSP_LED_Off(0u);
                BSP_LED_On(AppRollingLed);
                AppVisibleLed = AppRollingLed;
                AppRollingLed++;
                if (AppRollingLed > APP_LED_3) {
                    AppRollingLed = APP_LED_1;
                }
            }
        }

        OSTimeDlyHMSM(0u, 0u, 0u, APP_OUTPUT_POLL_MS,
                      OS_OPT_TIME_HMSM_STRICT,
                      &err);
        led_elapsed_ms += APP_OUTPUT_POLL_MS;
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

__attribute__((unused)) static void AppObjCreate(void)
{
    OS_ERR err;

    OSSemCreate(&AppButtonSem,
                "Button Semaphore",
                0u,
                &err);

    OSQCreate(&AppEventQ,
              "Application Event Queue",
              APP_EVENT_Q_SIZE,
              &err);
}

/*
*********************************************************************************************************
* TASK CREATE
*********************************************************************************************************
*/

__attribute__((unused)) static void AppTaskCreate(void)
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

static void AppUsart3Init(void)
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
#if !APP_USE_PARKING
    OS_ERR err;
#endif

    if (EXTI_GetITStatus(EXTI_Line13) != RESET) {
        EXTI_ClearITPendingBit(EXTI_Line13);
#if APP_USE_PARKING
        ButtonTask_SignalFromISR();
#else
        OSSemPost(&AppButtonSem,
                  OS_OPT_POST_1,
                  &err);
#endif
    }
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

    CPU_SR_ALLOC();

    OS_CRITICAL_ENTER();
    p_event = &AppEventPool[AppEventWrIx];
    AppEventWrIx++;
    if (AppEventWrIx >= APP_EVENT_Q_SIZE) {
        AppEventWrIx = 0u;
    }
    OS_CRITICAL_EXIT();

    p_event->Type   = type;
    p_event->Target = target;
    p_event->Value  = value;

    OSQPost(&AppEventQ,
            (void *)p_event,
            (OS_MSG_SIZE)sizeof(APP_EVENT),
            OS_OPT_POST_FIFO,
            &err);
}

static void AppCmdParse(CPU_CHAR *line)
{
    AppTrace("[USART] cmd: ");
    AppTrace(line);
    AppTrace("\r\n");

    if (AppStrEq(line, "help") == DEF_TRUE) {
        AppEventPost(APP_EVENT_CMD_HELP, 0u, 0u);
    } else if (AppStrEq(line, "status") == DEF_TRUE) {
        AppEventPost(APP_EVENT_CMD_STATUS, 0u, 0u);
    } else if (AppStrEq(line, "mode rolling") == DEF_TRUE) {
        AppEventPost(APP_EVENT_CMD_MODE_ROLLING, 0u, 0u);
    } else if (AppStrEq(line, "mode blink") == DEF_TRUE) {
        AppEventPost(APP_EVENT_CMD_MODE_BLINK, 0u, 0u);
    } else if (AppStrEq(line, "mode off") == DEF_TRUE) {
        AppEventPost(APP_EVENT_CMD_MODE_OFF, 0u, 0u);
    } else if (AppStrEq(line, "target1") == DEF_TRUE) {
        AppEventPost(APP_EVENT_CMD_TARGET, APP_LED_1, 0u);
    } else if (AppStrEq(line, "target2") == DEF_TRUE) {
        AppEventPost(APP_EVENT_CMD_TARGET, APP_LED_2, 0u);
    } else if (AppStrEq(line, "target3") == DEF_TRUE) {
        AppEventPost(APP_EVENT_CMD_TARGET, APP_LED_3, 0u);
    } else if (AppStrEq(line, "reset") == DEF_TRUE) {
        AppEventPost(APP_EVENT_CMD_RESET, 0u, 0u);
    } else {
        AppTrace("[USART] unknown command\r\n");
        AppPrintHelp();
    }
}

static void AppPrintHelp(void)
{
    AppTrace("[HELP] help | status | mode rolling | mode blink | mode off | target1 | target2 | target3 | reset\r\n");
}

static void AppPrintStatus(void)
{
    AppTrace("[MONITOR] mode=");
    AppTraceUInt((CPU_INT32U)AppLedMode);
    AppTrace(" target=LED");
    AppTraceUInt((CPU_INT32U)AppTargetLed);
    AppTrace(" rolling=LED");
    AppTraceUInt((CPU_INT32U)AppRollingLed);
    AppTrace(" buttons=");
    AppTraceUInt(AppButtonCount);
    AppTrace(" cmds=");
    AppTraceUInt(AppCommandCount);
    AppTrace("\r\n");
}

/*
*********************************************************************************************************
* USART / STRING HELPERS
*********************************************************************************************************
*/

static void AppTrace(const CPU_CHAR *msg)
{
    while (*msg != (CPU_CHAR)0) {
        AppTraceChar(*msg);
        msg++;
    }
}

static void AppTraceChar(CPU_CHAR ch)
{
    while (USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET) {
    }
    USART_SendData(USART3, (uint16_t)ch);
}

static void AppTraceUInt(CPU_INT32U value)
{
    CPU_CHAR   buf[11];
    CPU_INT08U ix = 0u;

    if (value == 0u) {
        AppTraceChar('0');
        return;
    }

    while ((value > 0u) && (ix < sizeof(buf))) {
        buf[ix] = (CPU_CHAR)('0' + (value % 10u));
        value /= 10u;
        ix++;
    }

    while (ix > 0u) {
        ix--;
        AppTraceChar(buf[ix]);
    }
}

static CPU_BOOLEAN AppUsartTryGetChar(CPU_CHAR *ch)
{
    if (USART_GetFlagStatus(USART3, USART_FLAG_RXNE) == RESET) {
        return DEF_FALSE;
    }

    *ch = (CPU_CHAR)(USART_ReceiveData(USART3) & 0xFFu);
    return DEF_TRUE;
}

static CPU_BOOLEAN AppUsartTryGetLine(CPU_CHAR *buf, CPU_INT16U buf_len)
{
    static CPU_CHAR   line[APP_CMD_LINE_LEN];
    static CPU_INT16U line_len = 0u;
    CPU_CHAR          ch;
    CPU_INT16U        ix;

    if (AppUsartTryGetChar(&ch) == DEF_FALSE) {
        return DEF_FALSE;
    }

    if ((ch == '\r') || (ch == '\n')) {
        AppTrace("\r\n");
        if (line_len == 0u) {
            return DEF_FALSE;
        }

        line[line_len] = (CPU_CHAR)0;
        for (ix = 0u; (ix < line_len) && (ix < (buf_len - 1u)); ix++) {
            buf[ix] = line[ix];
        }
        buf[ix] = (CPU_CHAR)0;
        line_len = 0u;
        return DEF_TRUE;
    }

    if ((ch == '\b') || (ch == 0x7Fu)) {
        if (line_len > 0u) {
            line_len--;
            AppTrace("\b \b");
        }
        return DEF_FALSE;
    }

    if (line_len >= (APP_CMD_LINE_LEN - 1u)) {
        line_len = 0u;
        AppTrace("\r\n[USART] command too long\r\n");
        return DEF_FALSE;
    }

    line[line_len] = ch;
    line_len++;
    AppTraceChar(ch);

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
