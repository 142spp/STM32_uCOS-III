/*
*********************************************************************************************************
* Smart Parking - drv_hcsr04.c
*
* HC-SR04 x2. echo 펄스 폭을 TIM2 입력 캡처 인터럽트로 측정한다.
*   TRIG : PE0 (sensor0), PE11=Arduino D5 (sensor1)  - GPIO 출력
*   ECHO : PA0 (TIM2_CH1), PA3 (TIM2_CH4) - AF1, 5V이므로 분압 회로 필수
*          (PA1은 NUCLEO-F429ZI 온보드 이더넷 RMII_REF_CLK이라 사용 불가 -> PA3로)
*
* TIM2(32-bit)를 1us/tick로 구동. ISR은 에지 타임스탬프 캡처와 거리 환산만 하고
* OS는 호출하지 않는다(RTOS 비의존). 대기는 호출 task가 OSTimeDly로 담당한다.
*********************************************************************************************************
*/

#include  "drv_hcsr04.h"
#include  "stm32f4xx.h"
#include  "stm32f4xx_rcc.h"
#include  "stm32f4xx_gpio.h"
#include  "stm32f4xx_tim.h"
#include  "misc.h"
#include  "bsp.h"

#define  HCSR04_SENSOR_COUNT     2u
#define  HCSR04_ECHO_TIMEOUT_US  25000u         /* ~4.3m 초과는 무효 처리                              */

/* HCLK 168MHz -> APB1 42MHz -> APB1 타이머(TIM2) 84MHz. /84 = 1MHz(1us/tick) */
#define  HCSR04_TIM_PSC          (84u - 1u)

static const uint16_t HcTrigPin[HCSR04_SENSOR_COUNT]    = { GPIO_Pin_0,    GPIO_Pin_11   };
static const uint16_t HcEchoChannel[HCSR04_SENSOR_COUNT] = { TIM_Channel_1, TIM_Channel_4 };
static const uint16_t HcEchoIt[HCSR04_SENSOR_COUNT]      = { TIM_IT_CC1,    TIM_IT_CC4    };
static const uint16_t HcEchoFlag[HCSR04_SENSOR_COUNT]    = { TIM_FLAG_CC1,  TIM_FLAG_CC4  };
static const uint16_t HcEchoOfFlag[HCSR04_SENSOR_COUNT]  = { TIM_FLAG_CC1OF, TIM_FLAG_CC4OF };

static volatile uint32_t HcCaptureStart[HCSR04_SENSOR_COUNT];
static volatile uint16_t HcDistanceMm[HCSR04_SENSOR_COUNT];
static volatile uint8_t  HcEdgeRising[HCSR04_SENSOR_COUNT];
static volatile uint8_t  HcReady[HCSR04_SENSOR_COUNT];

/*
*********************************************************************************************************
* LOCAL HELPERS
*********************************************************************************************************
*/

static void HCSR04_SetEdge(uint16_t channel, uint16_t polarity)
{
    TIM_ICInitTypeDef ic;

    ic.TIM_Channel     = channel;
    ic.TIM_ICPolarity  = polarity;
    ic.TIM_ICSelection = TIM_ICSelection_DirectTI;
    ic.TIM_ICPrescaler = TIM_ICPSC_DIV1;
    ic.TIM_ICFilter    = 0u;
    TIM_ICInit(TIM2, &ic);
}

static void HCSR04_DelayUs(uint32_t us)
{
    uint32_t t0 = TIM_GetCounter(TIM2);
    while ((TIM_GetCounter(TIM2) - t0) < us) {
    }
}

static void HCSR04_HandleCapture(uint8_t id, uint16_t channel, uint32_t ts)
{
    uint32_t width;

    if (HcEdgeRising[id] != 0u) {
        HcCaptureStart[id] = ts;
        HcEdgeRising[id]   = 0u;
        HCSR04_SetEdge(channel, TIM_ICPolarity_Falling);
    } else {
        width = ts - HcCaptureStart[id];                 /* us, unsigned 랩어라운드 안전 */
        if (width > HCSR04_ECHO_TIMEOUT_US) {
            HcDistanceMm[id] = HCSR04_DISTANCE_INVALID;
        } else {
            HcDistanceMm[id] = (uint16_t)((width * 343u) / 2000u);   /* mm = us*0.1715 */
        }
        HcReady[id]      = 1u;
        HcEdgeRising[id] = 1u;
        TIM_ITConfig(TIM2, (channel == TIM_Channel_1) ? TIM_IT_CC1 : TIM_IT_CC4, DISABLE);
        HCSR04_SetEdge(channel, TIM_ICPolarity_Rising);
    }
}

/* TIM2 글로벌 인터럽트 (BSP_IntVectSet으로 등록) */
static void HCSR04_IC_ISR(void)
{
    if (TIM_GetITStatus(TIM2, TIM_IT_CC1) != RESET) {
        TIM_ClearITPendingBit(TIM2, TIM_IT_CC1);
        HCSR04_HandleCapture(0u, TIM_Channel_1, TIM_GetCapture1(TIM2));
    }
    if (TIM_GetITStatus(TIM2, TIM_IT_CC4) != RESET) {
        TIM_ClearITPendingBit(TIM2, TIM_IT_CC4);
        HCSR04_HandleCapture(1u, TIM_Channel_4, TIM_GetCapture4(TIM2));
    }

    TIM_ClearITPendingBit(TIM2, TIM_IT_Update | TIM_IT_CC2 |
                                TIM_IT_CC3    | TIM_IT_Trigger);
    TIM_ClearFlag(TIM2, TIM_FLAG_CC1OF | TIM_FLAG_CC4OF);
}

/*
*********************************************************************************************************
* PUBLIC
*********************************************************************************************************
*/

void HCSR04_Init(void)
{
    GPIO_InitTypeDef        gpio;
    TIM_TimeBaseInitTypeDef tb;
    uint8_t                 i;

    for (i = 0u; i < HCSR04_SENSOR_COUNT; i++) {
        HcEdgeRising[i] = 1u;
        HcReady[i]      = 0u;
        HcDistanceMm[i] = HCSR04_DISTANCE_INVALID;
    }

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOE, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    /* TRIG: PE0(sensor0), PE11=Arduino D5(sensor1) 출력 */
    gpio.GPIO_Pin   = GPIO_Pin_0 | GPIO_Pin_11;
    gpio.GPIO_Mode  = GPIO_Mode_OUT;
    gpio.GPIO_OType = GPIO_OType_PP;
    gpio.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOE, &gpio);
    GPIO_ResetBits(GPIOE, GPIO_Pin_0 | GPIO_Pin_11);

    /* ECHO: PA0(CH1), PA3(CH4) -> TIM2 AF1 */
    gpio.GPIO_Pin   = GPIO_Pin_0 | GPIO_Pin_3;
    gpio.GPIO_Mode  = GPIO_Mode_AF;
    gpio.GPIO_OType = GPIO_OType_PP;
    gpio.GPIO_PuPd  = GPIO_PuPd_DOWN;            /* echo 평상시 LOW */
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource0, GPIO_AF_TIM2);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_TIM2);

    /* TIM2: 1us/tick, 32-bit 자유 구동 */
    tb.TIM_Prescaler         = HCSR04_TIM_PSC;
    tb.TIM_CounterMode       = TIM_CounterMode_Up;
    tb.TIM_Period            = 0xFFFFFFFFu;
    tb.TIM_ClockDivision     = TIM_CKD_DIV1;
    tb.TIM_RepetitionCounter = 0u;
    TIM_TimeBaseInit(TIM2, &tb);

    HCSR04_SetEdge(TIM_Channel_1, TIM_ICPolarity_Rising);
    HCSR04_SetEdge(TIM_Channel_4, TIM_ICPolarity_Rising);

    TIM_ITConfig(TIM2, TIM_IT_CC1 | TIM_IT_CC4, DISABLE);
    TIM_Cmd(TIM2, ENABLE);

    TIM_ClearFlag(TIM2, TIM_FLAG_Update | TIM_FLAG_CC1 | TIM_FLAG_CC2 |
                        TIM_FLAG_CC3    | TIM_FLAG_CC4 | TIM_FLAG_Trigger |
                        TIM_FLAG_CC1OF  | TIM_FLAG_CC4OF);
    TIM_ClearITPendingBit(TIM2, TIM_IT_Update | TIM_IT_CC1 | TIM_IT_CC2 |
                                TIM_IT_CC3    | TIM_IT_CC4 | TIM_IT_Trigger);
    NVIC_ClearPendingIRQ(TIM2_IRQn);

    BSP_IntVectSet(BSP_INT_ID_TIM2, HCSR04_IC_ISR);
    BSP_IntPrioSet(BSP_INT_ID_TIM2, 5u);
    BSP_IntEn(BSP_INT_ID_TIM2);
}

void HCSR04_Trigger(uint8_t sensor_id)
{
    if (sensor_id >= HCSR04_SENSOR_COUNT) {
        return;
    }

    HcReady[sensor_id]      = 0u;
    HcEdgeRising[sensor_id] = 1u;                            /* 다음 캡처는 상승부터 */
    HCSR04_SetEdge(HcEchoChannel[sensor_id], TIM_ICPolarity_Rising);
    TIM_ClearFlag(TIM2, HcEchoFlag[sensor_id] | HcEchoOfFlag[sensor_id]);
    TIM_ClearITPendingBit(TIM2, HcEchoIt[sensor_id]);
    NVIC_ClearPendingIRQ(TIM2_IRQn);
    TIM_ITConfig(TIM2, HcEchoIt[sensor_id], ENABLE);

    GPIO_SetBits(GPIOE, HcTrigPin[sensor_id]);
    HCSR04_DelayUs(10u);
    GPIO_ResetBits(GPIOE, HcTrigPin[sensor_id]);
}

uint16_t HCSR04_ReadDistance(uint8_t sensor_id)
{
    if (sensor_id >= HCSR04_SENSOR_COUNT) {
        return HCSR04_DISTANCE_INVALID;
    }
    if (HcReady[sensor_id] == 0u) {
        TIM_ITConfig(TIM2, HcEchoIt[sensor_id], DISABLE);
        TIM_ClearFlag(TIM2, HcEchoFlag[sensor_id] | HcEchoOfFlag[sensor_id]);
        return HCSR04_DISTANCE_INVALID;                     /* echo 미수신 = 타임아웃 */
    }
    return HcDistanceMm[sensor_id];
}
