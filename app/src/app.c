/*
*********************************************************************************************************
* Smart Parking - app.c
*
* OS/보드 부팅과 Smart Parking 애플리케이션 진입만 담당한다.
* 실제 시스템 로직은 tasks/ 와 drivers/ 에 있으며, ParkingApp_Init()으로 생성된다.
*********************************************************************************************************
*/

#include  <includes.h>
#include  "stm32f4xx.h"
#include  "stm32f4xx_rcc.h"
#include  "stm32f4xx_gpio.h"
#include  "stm32f4xx_exti.h"
#include  "stm32f4xx_syscfg.h"
#include  "misc.h"

#include  "app_parking.h"
#include  "task_button.h"
#include  "drv_usart.h"

/*
*********************************************************************************************************
* FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void  AppTaskStart        (void *p_arg);
static  void  AppButtonExtiInit   (void);
static  void  AppButtonISR        (void);

/*
*********************************************************************************************************
* GLOBAL VARIABLES
*********************************************************************************************************
*/

static  OS_TCB   AppTaskStartTCB;
static  CPU_STK  AppTaskStartStk[APP_CFG_TASK_START_STK_SIZE];

/*
*********************************************************************************************************
* main()
*********************************************************************************************************
*/

int main(void)
{
    OS_ERR  err;

    RCC_DeInit();

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
*
* 보드/주변장치를 초기화한 뒤 Smart Parking task들을 생성한다.
*********************************************************************************************************
*/

static void AppTaskStart(void *p_arg)
{
    OS_ERR err;

    (void)p_arg;

    BSP_Init();
    BSP_Tick_Init();
    Usart_Init();

#if OS_CFG_STAT_TASK_EN > 0u
    OSStatTaskCPUUsageInit(&err);
#endif

#ifdef CPU_CFG_INT_DIS_MEAS_EN
    CPU_IntDisMeasMaxCurReset();
#endif

    Usart_PutStr("\r\n[BOOT] Smart Parking\r\n");

    ParkingApp_Init();          /* IPC + 전체 task 생성 (ButtonTask가 ButtonSem 생성) */
    AppButtonExtiInit();         /* 출차 버튼 EXTI: ISR이 ButtonTask를 깨운다 */

    while (DEF_TRUE) {
        OSTimeDlyHMSM(0u, 0u, 1u, 0u,
                      OS_OPT_TIME_HMSM_STRICT,
                      &err);
    }
}

/*
*********************************************************************************************************
* 출차 버튼 EXTI (PC13, falling edge)
*   ISR은 ButtonTask를 깨우기만 하고 실제 처리는 task에서 수행한다.
*********************************************************************************************************
*/

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
    if (EXTI_GetITStatus(EXTI_Line13) != RESET) {
        EXTI_ClearITPendingBit(EXTI_Line13);
        ButtonTask_SignalFromISR();
    }
}
