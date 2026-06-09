/*
*********************************************************************************************************
*                                             HW#5 STARTER CODE
*
*                                  uC/OS-III USART LED Command Control
*
* Board assumption:
*   USART3 TX : PD8
*   USART3 RX : PD9
*   LED1      : PB0
*   LED2      : PB7
*   LED3      : PB14
*
* Starter code provides uC/OS startup, USART3/GPIO setup, and USART output.
*********************************************************************************************************
*/

#include <includes.h>
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
#define CMD_BUF_SIZE            32u
#define LED_COUNT                3u
#define APP_TASK_USART_PRIO      3u
#define APP_TASK_LED_PRIO        4u

/*
*********************************************************************************************************
*                                            LOCAL TYPES
*********************************************************************************************************
*/

typedef enum {
    LED_MODE_OFF,
    LED_MODE_ON,
    LED_MODE_BLINK
} led_mode_t;

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

static CPU_BOOLEAN UsartGetChar(char *c);
static void UsartPutChar(char c);
static void UsartPrint(const char *s);

static void AppInit(void);
static void HandleInputChar(char ch);
static void HandleCommand(char *cmd);
static void LedTaskTick(void);
static void PrintHelp(void);
static void PrintPrompt(void);
static void PrintStatus(void);
static void SetLed(CPU_INT08U led, led_mode_t mode);
static CPU_BOOLEAN ParseLed(const char *token, CPU_INT08U *led);
static char *NextToken(char **cursor);

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

static char        CmdBuf[CMD_BUF_SIZE];
static CPU_INT08U  CmdLen;
static led_mode_t  LedModes[LED_COUNT];
static CPU_BOOLEAN BlinkOn;

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
    OS_ERR err;

    (void)p_arg;

    BSP_Init();
    BSP_Tick_Init();
    Setup_Usart3();
    AppInit();

#if OS_CFG_STAT_TASK_EN > 0u
    OSStatTaskCPUUsageInit(&err);
#else
    (void)err;
#endif

#ifdef CPU_CFG_INT_DIS_MEAS_EN
    CPU_IntDisMeasMaxCurReset();
#endif

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
                 "USART Command Task",
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
                 "LED Control Task",
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
    char   ch;

    (void)p_arg;

    while (DEF_TRUE) {
        while (UsartGetChar(&ch) == DEF_TRUE) {
            HandleInputChar(ch);
        }

        OSTimeDlyHMSM(0u, 0u, 0u, 10u,
                      OS_OPT_TIME_HMSM_STRICT,
                      &err);
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

    (void)p_arg;

    while (DEF_TRUE) {
        LedTaskTick();

        OSTimeDlyHMSM(0u, 0u, 0u, 500u,
                      OS_OPT_TIME_HMSM_STRICT,
                      &err);
    }
}

/*
*********************************************************************************************************
*                                          APPLICATION LOGIC
*********************************************************************************************************
*/

static void AppInit(void)
{
    CPU_INT08U led;

    CmdLen  = 0u;
    BlinkOn = DEF_FALSE;

    for (led = 1u; led <= LED_COUNT; led++) {
        SetLed(led, LED_MODE_OFF);
    }

    UsartPrint("\r\nHW#5 USART LED Command Control\r\n");
    PrintHelp();
    PrintPrompt();
}

static void HandleInputChar(char ch)
{
    if ((ch == '\r') || (ch == '\n')) {
        UsartPrint("\r\n");

        if (CmdLen > 0u) {
            CmdBuf[CmdLen] = '\0';
            HandleCommand(CmdBuf);
            CmdLen = 0u;
        }

        PrintPrompt();
        return;
    }

    if ((ch == '\b') || (ch == 0x7f)) {
        if (CmdLen > 0u) {
            CmdLen--;
            UsartPrint("\b \b");
        }
        return;
    }

    if (!isprint((int)ch)) {
        return;
    }

    if (CmdLen >= (CMD_BUF_SIZE - 1u)) {
        UsartPrint("\r\nerror: command too long\r\n");
        CmdLen = 0u;
        PrintPrompt();
        return;
    }

    CmdBuf[CmdLen++] = (char)tolower((int)ch);
    UsartPutChar(ch);
}

/*
*********************************************************************************************************
*                                          COMMAND PARSER
*********************************************************************************************************
*/

static void HandleCommand(char *cmd)
{
    char       *cursor = cmd;
    char       *arg0;
    char       *arg1;
    CPU_INT08U  led;

    arg0 = NextToken(&cursor);
    if (arg0 == (char *)0) {
        return;
    }

    if ((strcmp(arg0, "help") == 0) || (strcmp(arg0, "?") == 0)) {
        PrintHelp();
        return;
    }

    if (strcmp(arg0, "status") == 0) {
        PrintStatus();
        return;
    }

    if (strcmp(arg0, "all") == 0) {
        arg1 = NextToken(&cursor);
        if (arg1 == (char *)0) {
            UsartPrint("usage: all on|off|blink\r\n");
            return;
        }

        for (led = 1u; led <= LED_COUNT; led++) {
            if (strcmp(arg1, "on") == 0) {
                SetLed(led, LED_MODE_ON);
            } else if (strcmp(arg1, "off") == 0) {
                SetLed(led, LED_MODE_OFF);
            } else if (strcmp(arg1, "blink") == 0) {
                SetLed(led, LED_MODE_BLINK);
            } else {
                UsartPrint("usage: all on|off|blink\r\n");
                return;
            }
        }

        PrintStatus();
        return;
    }

    if (ParseLed(arg0, &led) == DEF_TRUE) {
        arg1 = NextToken(&cursor);
        if (arg1 == (char *)0) {
            UsartPrint("usage: led1|led2|led3 on|off|toggle|blink\r\n");
            return;
        }

        if (strcmp(arg1, "on") == 0) {
            SetLed(led, LED_MODE_ON);
        } else if (strcmp(arg1, "off") == 0) {
            SetLed(led, LED_MODE_OFF);
        } else if (strcmp(arg1, "toggle") == 0) {
            if (LedModes[led - 1u] == LED_MODE_OFF) {
                SetLed(led, LED_MODE_ON);
            } else {
                SetLed(led, LED_MODE_OFF);
            }
        } else if (strcmp(arg1, "blink") == 0) {
            SetLed(led, LED_MODE_BLINK);
        } else {
            UsartPrint("usage: led1|led2|led3 on|off|toggle|blink\r\n");
            return;
        }

        PrintStatus();
        return;
    }

    UsartPrint("unknown command\r\n");
    PrintHelp();
}

static void LedTaskTick(void)
{
    CPU_INT08U led;

    BlinkOn = (BlinkOn == DEF_TRUE) ? DEF_FALSE : DEF_TRUE;

    for (led = 1u; led <= LED_COUNT; led++) {
        if (LedModes[led - 1u] == LED_MODE_BLINK) {
            if (BlinkOn == DEF_TRUE) {
                BSP_LED_On(led);
            } else {
                BSP_LED_Off(led);
            }
        }
    }
}

static void PrintHelp(void)
{
    UsartPrint("commands:\r\n");
    UsartPrint("  led1 on|off|toggle|blink\r\n");
    UsartPrint("  led2 on|off|toggle|blink\r\n");
    UsartPrint("  led3 on|off|toggle|blink\r\n");
    UsartPrint("  all on|off|blink\r\n");
    UsartPrint("  status\r\n");
}

static void PrintPrompt(void)
{
    UsartPrint("> ");
}

static void PrintStatus(void)
{
    CPU_INT08U led;

    UsartPrint("status:");
    for (led = 1u; led <= LED_COUNT; led++) {
        UsartPrint(" led");
        UsartPutChar((char)('0' + led));
        UsartPutChar('=');

        switch (LedModes[led - 1u]) {
            case LED_MODE_ON:
                UsartPrint("on");
                break;

            case LED_MODE_BLINK:
                UsartPrint("blink");
                break;

            case LED_MODE_OFF:
            default:
                UsartPrint("off");
                break;
        }
    }
    UsartPrint("\r\n");
}

static void SetLed(CPU_INT08U led, led_mode_t mode)
{
    if ((led == 0u) || (led > LED_COUNT)) {
        return;
    }

    LedModes[led - 1u] = mode;

    if (mode == LED_MODE_ON) {
        BSP_LED_On(led);
    } else if (mode == LED_MODE_OFF) {
        BSP_LED_Off(led);
    }
}

static CPU_BOOLEAN ParseLed(const char *token, CPU_INT08U *led)
{
    if ((token == (const char *)0) || (led == (CPU_INT08U *)0)) {
        return DEF_FALSE;
    }

    if ((strncmp(token, "led", 3u) == 0) &&
        (token[3] >= '1') &&
        (token[3] <= (char)('0' + LED_COUNT)) &&
        (token[4] == '\0')) {
        *led = (CPU_INT08U)(token[3] - '0');
        return DEF_TRUE;
    }

    return DEF_FALSE;
}

static char *NextToken(char **cursor)
{
    char *token;

    if ((cursor == (char **)0) || (*cursor == (char *)0)) {
        return (char *)0;
    }

    while ((**cursor != '\0') && isspace((int)**cursor)) {
        (*cursor)++;
    }

    if (**cursor == '\0') {
        return (char *)0;
    }

    token = *cursor;

    while ((**cursor != '\0') && !isspace((int)**cursor)) {
        (*cursor)++;
    }

    if (**cursor != '\0') {
        **cursor = '\0';
        (*cursor)++;
    }

    return token;
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
*                                          USART HELPERS
*********************************************************************************************************
*/

static CPU_BOOLEAN UsartGetChar(char *c)
{
    if (USART_GetFlagStatus(USART3, USART_FLAG_RXNE) == RESET) {
        return DEF_FALSE;
    }

    *c = (char)(USART_ReceiveData(USART3) & 0xFFu);
    return DEF_TRUE;
}

static void UsartPutChar(char c)
{
    while (USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET) {
    }

    USART_SendData(USART3, (uint16_t)c);
}

static void UsartPrint(const char *s)
{
    while (*s != '\0') {
        UsartPutChar(*s++);
    }
}
