/*
*********************************************************************************************************
* Smart Parking - drv_selftest.c
*
* 한 번에 한 드라이버만 단독으로 돌려 USART/육안으로 검증한다.
* 아래 SELFTEST_TARGET 을 원하는 값으로 바꾸고 빌드/플래시한 뒤, 시리얼 터미널(115200 8N1)을 본다.
*********************************************************************************************************
*/

#include  <includes.h>
#include  "drv_selftest.h"
#include  "app_types.h"

#include  "drv_usart.h"
#include  "drv_hcsr04.h"
#include  "drv_servo.h"
#include  "drv_lcd1602.h"
#include  "drv_ir.h"
#include  "drv_buzzer.h"

/*
*********************************************************************************************************
* 테스트 대상 선택 (이 값 하나만 바꾸면 된다)
*********************************************************************************************************
*/

#define  ST_HCSR04   1
#define  ST_SERVO    2
#define  ST_LCD      3
#define  ST_IR       4
#define  ST_BUZZER   5

#define  SELFTEST_TARGET   ST_SERVO

/*
*********************************************************************************************************
* HELPERS
*********************************************************************************************************
*/

/* HMSM STRICT 모드는 ms가 0~999만 허용하므로 초/밀리초로 쪼개 넘긴다. */
static void ST_Dly(CPU_INT16U ms)
{
    OS_ERR err;
    OSTimeDlyHMSM(0u, 0u,
                  (CPU_INT16U)(ms / 1000u),
                  (CPU_INT16U)(ms % 1000u),
                  OS_OPT_TIME_HMSM_STRICT, &err);
}

/*
*********************************************************************************************************
* TEST ROUTINES
*********************************************************************************************************
*/

#if   (SELFTEST_TARGET == ST_HCSR04)
/* 거리값 출력: 실패 시 INVALID, 아니면 "<n>mm" */
static void ST_PrintDist(CPU_INT08U id, CPU_INT16U dist)
{
    Usart_PutStr(" S");
    Usart_PutUInt(id);
    Usart_PutStr(":");
    if (dist == HCSR04_DISTANCE_INVALID) {
        Usart_PutStr("INVALID");
    } else {
        Usart_PutUInt(dist);
        Usart_PutStr("mm");
    }
}

/* 손/장애물을 센서 앞에 대보면 거리값이 변해야 한다. 둘 다 정상값(보통 20~4000mm) 나오는지 확인. */
static void ST_RunHcsr04(void)
{
    CPU_INT08U slot;
    CPU_INT16U dist;

    Usart_PutStr("[SELFTEST] HC-SR04 x2  (S0=PE0/PA0, S1=PE11/PA3)\r\n");
    HCSR04_Init();

    while (DEF_TRUE) {
        Usart_PutStr("dist");
        for (slot = 0u; slot < APP_SLOT_COUNT; slot++) {
            HCSR04_Trigger(slot);
            ST_Dly(60u);                        /* echo 왕복 대기 */
            dist = HCSR04_ReadDistance(slot);
            ST_PrintDist(slot, dist);
        }
        Usart_PutStr("\r\n");
        ST_Dly(300u);
    }
}

#elif (SELFTEST_TARGET == ST_SERVO)
/* 차단기 서보가 1.5초마다 열림/닫힘을 왕복해야 한다. */
static void ST_RunServo(void)
{
    Usart_PutStr("[SELFTEST] Servo (PA6/TIM3_CH1)\r\n");
    Servo_Init();
    while (DEF_TRUE) {
        Usart_PutStr("open\r\n");
        Servo_Open();
        ST_Dly(1500u);
        Usart_PutStr("close\r\n");
        Servo_Close();
        ST_Dly(1500u);
    }
}

#elif (SELFTEST_TARGET == ST_LCD)
/* LCD에 고정 문구 + 증가 카운터가 보여야 한다. 안 보이면 I2C 주소(0x27/0x3F) 확인. */
static void ST_RunLcd(void)
{
    CPU_INT32U n = 0u;
    char       line[17];
    CPU_INT08U i;

    Usart_PutStr("[SELFTEST] LCD1602 (I2C1 PB6/PB7)\r\n");
    LCD_Init();
    LCD_Clear();
    LCD_Print(0u, "SelfTest LCD OK");

    while (DEF_TRUE) {
        /* "count: <n>" 를 단순 포맷 (16자 패딩) */
        const char *pfx = "count:";
        for (i = 0u; (pfx[i] != '\0') && (i < 16u); i++) {
            line[i] = pfx[i];
        }
        if (i < 16u) { line[i++] = ' '; }
        /* n을 자리수대로 (간단히 한 자리~ 출력은 Usart로, LCD엔 마지막 숫자만) */
        if (i < 16u) { line[i++] = (char)('0' + (n % 10u)); }
        for (; i < 16u; i++) { line[i] = ' '; }
        line[16] = '\0';
        LCD_Print(1u, line);

        Usart_PutStr("lcd tick ");
        Usart_PutUInt(n);
        Usart_PutStr("\r\n");
        n++;
        ST_Dly(500u);
    }
}

#elif (SELFTEST_TARGET == ST_IR)
/* 손을 센서 앞에 대면 1, 치우면 0 이 떠야 한다 (active-low 모듈 기준). */
static void ST_RunIr(void)
{
    Usart_PutStr("[SELFTEST] IR (PC0)\r\n");
    IR_Init();
    while (DEF_TRUE) {
        Usart_PutStr("detected=");
        Usart_PutUInt(IR_IsDetected());
        Usart_PutStr("\r\n");
        ST_Dly(200u);
    }
}

#elif (SELFTEST_TARGET == ST_BUZZER)
/* 0.3초 울리고 0.7초 쉬는 것을 반복. 소리가 나야 한다 (수동 부저). */
static void ST_RunBuzzer(void)
{
    Usart_PutStr("[SELFTEST] Buzzer (PD12/TIM4_CH1)\r\n");
    Buzzer_Init();
    while (DEF_TRUE) {
        Usart_PutStr("beep\r\n");
        Buzzer_On();
        ST_Dly(300u);
        Buzzer_Off();
        ST_Dly(700u);
    }
}
#endif

/*
*********************************************************************************************************
* ENTRY
*********************************************************************************************************
*/

void DrvSelfTest_Run(void)
{
#if   (SELFTEST_TARGET == ST_HCSR04)
    ST_RunHcsr04();
#elif (SELFTEST_TARGET == ST_SERVO)
    ST_RunServo();
#elif (SELFTEST_TARGET == ST_LCD)
    ST_RunLcd();
#elif (SELFTEST_TARGET == ST_IR)
    ST_RunIr();
#elif (SELFTEST_TARGET == ST_BUZZER)
    ST_RunBuzzer();
#endif
}
