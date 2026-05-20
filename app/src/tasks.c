#include  <includes.h>
#include  "serial.h"
#include  "tasks.h"

typedef enum {
    TASK_SERIAL_RX,
    TASK_500MS,
    TASK_1000MS,
    TASK_2000MS,

    TASK_N
} task_e;

typedef struct {
    CPU_CHAR   *name;
    OS_TASK_PTR func;
    OS_PRIO     prio;
    CPU_STK    *pStack;
    OS_TCB     *pTcb;
} task_t;

static void AppTask_500ms(void *p_arg);
static void AppTask_1000ms(void *p_arg);
static void AppTask_2000ms(void *p_arg);
static void AppTask_SerialRx(void *p_arg);

static OS_TCB Task_500ms_TCB;
static OS_TCB Task_1000ms_TCB;
static OS_TCB Task_2000ms_TCB;
static OS_TCB Task_SerialRx_TCB;

static CPU_STK Task_500ms_Stack[APP_CFG_TASK_START_STK_SIZE];
static CPU_STK Task_1000ms_Stack[APP_CFG_TASK_START_STK_SIZE];
static CPU_STK Task_2000ms_Stack[APP_CFG_TASK_START_STK_SIZE];
static CPU_STK Task_SerialRx_Stack[APP_CFG_TASK_START_STK_SIZE];

volatile int count = 0;

static task_t cyclic_tasks[TASK_N] = {
    {"Task_SerialRx", AppTask_SerialRx, 0, &Task_SerialRx_Stack[0], &Task_SerialRx_TCB},
    {"Task_500ms"   , AppTask_500ms,    0, &Task_500ms_Stack[0],    &Task_500ms_TCB},
    {"Task_1000ms"  , AppTask_1000ms,   0, &Task_1000ms_Stack[0],   &Task_1000ms_TCB},
    {"Task_2000ms"  , AppTask_2000ms,   0, &Task_2000ms_Stack[0],   &Task_2000ms_TCB},
};

/*
*********************************************************************************************************
*                                          AppTask_500ms
*
* Description : Example of 500mS Task
*********************************************************************************************************
*/
static void AppTask_500ms(void *p_arg)
{
    OS_ERR err;

    (void)p_arg;

    BSP_LED_On(1);
    while (DEF_TRUE) {
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
*********************************************************************************************************
*/
static void AppTask_1000ms(void *p_arg)
{
    OS_ERR      err;
    CPU_INT32U tx_count = 0u;
    char        msg[64];

    (void)p_arg;

    BSP_LED_On(2);
    while (DEF_TRUE) {
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
*********************************************************************************************************
*/
static void AppTask_2000ms(void *p_arg)
{
    OS_ERR err;

    (void)p_arg;

    BSP_LED_On(3);
    while (DEF_TRUE) {
        BSP_LED_Toggle(3);

        OSTimeDlyHMSM(0u, 0u, 2u, 0u,
                      OS_OPT_TIME_HMSM_STRICT,
                      &err);
    }
}

static void AppTask_SerialRx(void *p_arg)
{
    OS_ERR err;
    char   ch;

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
*                                          AppTasks_Create()
*
* Description : Create application tasks.
*********************************************************************************************************
*/
void AppTasks_Create(void)
{
    OS_ERR     err;
    CPU_INT08U idx;

    for (idx = 0u; idx < TASK_N; idx++) {
        task_t *pTask_Cfg = &cyclic_tasks[idx];

        OSTaskCreate((OS_TCB *)         	pTask_Cfg->pTcb,
                     (CPU_CHAR *)    	pTask_Cfg->name,
                     (OS_TASK_PTR)      pTask_Cfg->func,
                     (void *)           	0u,
                     (OS_PRIO)        	pTask_Cfg->prio,
                     (CPU_STK *)    pTask_Cfg->pStack,
                     (CPU_STK_SIZE)  pTask_Cfg->pStack[APP_CFG_TASK_START_STK_SIZE / 10u],
                     (CPU_STK_SIZE)   APP_CFG_TASK_START_STK_SIZE,
                     (OS_MSG_QTY)       0u,
                     (OS_TICK)     0u,
                     (void *)         	0u,
                     (OS_OPT)         		(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                     (OS_ERR *)       	&err
					);
    }
}
