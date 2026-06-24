/*
*********************************************************************************************************
* Smart Parking - task_display.c
*
* DisplayQueue로 받은 상태를 1602 LCD와 RGB LED에 표시한다. LCD/LED 모두 이 task만 접근한다.
*   LED 색: 만차=빨강, 예약중=노랑, 빈자리=초록
*********************************************************************************************************
*/

#include  "task_display.h"
#include  "app_types.h"
#include  "ipc.h"
#include  "drv_lcd1602.h"
#include  "drv_led.h"

static  OS_TCB   DisplayTCB;
static  CPU_STK  DisplayStk[APP_TASK_STK_SIZE];

static char DispSlotChar(SLOT_STATE s)
{
    switch (s) {
        case SLOT_OCCUPIED: return 'O';
        case SLOT_RESERVED: return 'R';
        default:            return 'E';
    }
}

static CPU_INT08U DispAppend(char *dst, CPU_INT08U pos, const char *src)
{
    while ((*src != '\0') && (pos < LCD1602_COLS)) {
        dst[pos++] = *src++;
    }
    return pos;
}

/* 16자로 패딩하여 이전 출력 잔상을 덮어쓴다 */
static void DispPad(char *dst, CPU_INT08U pos)
{
    while (pos < LCD1602_COLS) {
        dst[pos++] = ' ';
    }
    dst[LCD1602_COLS] = '\0';
}

/* 현재 상태에 맞는 LED 색 결정: 만차=빨강, 예약중=노랑, 그 외=초록 */
static LED_COLOR DispLedColor(const DISPLAY_MSG *p)
{
    CPU_INT08U i;

    if (p->free_count == 0u) {
        return LED_RED;
    }
    for (i = 0u; i < APP_SLOT_COUNT; i++) {
        if (p->slot[i] == SLOT_RESERVED) {
            return LED_YELLOW;
        }
    }
    return LED_GREEN;
}

static void DisplayTask(void *p_arg)
{
    OS_ERR       err;
    OS_MSG_SIZE  size;
    DISPLAY_MSG *p;
    char         l0[LCD1602_COLS + 1u];
    char         l1[LCD1602_COLS + 1u];
    CPU_INT08U   pos;

    (void)p_arg;

    LCD_Init();
    LCD_Clear();
    LED_Init();

    while (DEF_TRUE) {
        p = (DISPLAY_MSG *)OSQPend(&DisplayQueue, 0u, OS_OPT_PEND_BLOCKING, &size, (CPU_TS *)0, &err);
        if ((err != OS_ERR_NONE) || (p == (DISPLAY_MSG *)0)) {
            continue;
        }

        /* line0: "FREE:x/y S1:c" (2칸 기준) */
        pos = DispAppend(l0, 0u, "FREE:");
        l0[pos++] = (char)('0' + p->free_count);
        l0[pos++] = '/';
        l0[pos++] = (char)('0' + APP_SLOT_COUNT);
        pos = DispAppend(l0, pos, " S1:");
        l0[pos++] = DispSlotChar(p->slot[0]);
        DispPad(l0, pos);

        /* line1: "S2:c G:OPEN/CLOSED" */
        pos = DispAppend(l1, 0u, "S2:");
        l1[pos++] = DispSlotChar(p->slot[1]);
        pos = DispAppend(l1, pos, " G:");
        pos = DispAppend(l1, pos, (p->gate_open != DEF_FALSE) ? "OPEN" : "CLOSED");
        DispPad(l1, pos);

        LCD_Print(0u, l0);
        LCD_Print(1u, l1);

        LED_Set(DispLedColor(p));               /* 상태색 갱신 */

        IPC_DisplayFree(p);
    }
}

void DisplayTask_Create(void)
{
    OS_ERR  err;

    OSTaskCreate(&DisplayTCB,
                 "Display Task",
                 DisplayTask,
                 (void *)0,
                 PRIO_DISPLAY,
                 &DisplayStk[0],
                 APP_TASK_STK_SIZE / 10u,
                 APP_TASK_STK_SIZE,
                 0u,
                 0u,
                 (void *)0,
                 OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR,
                 &err);
}
