/*
*********************************************************************************************************
* Smart Parking - drv_lcd1602.c
*
* 1602 Character LCD (I2C, PCF8574 백팩, HD44780 4비트). RTOS 비의존.
*   I2C1 : SCL=PB6, SDA=PB7 (AF4, open-drain)
*   PCF8574 비트맵: P0=RS, P1=RW, P2=E, P3=Backlight, P4..P7=D4..D7
* I2C 주소는 모듈에 따라 0x27 또는 0x3F. 실제 주소를 확인할 것.
* 타이밍 지연은 DWT 사이클 카운터 기반 busy-wait (OS 비의존, 초기화 단계에서도 동작).
*********************************************************************************************************
*/

#include  "drv_lcd1602.h"
#include  "stm32f4xx.h"
#include  "stm32f4xx_rcc.h"
#include  "stm32f4xx_gpio.h"
#include  "stm32f4xx_i2c.h"

#define  LCD_I2C_ADDR_7B    0x27u                /* 또는 0x3F                                          */
#define  LCD_I2C_ADDR_8B    (LCD_I2C_ADDR_7B << 1)

/* PCF8574 비트 */
#define  LCD_RS             0x01u
#define  LCD_RW             0x02u
#define  LCD_EN             0x04u
#define  LCD_BL             0x08u                /* 백라이트 ON                                         */

static uint8_t LcdBacklight = LCD_BL;

/*
*********************************************************************************************************
* DWT 사이클 카운터 기반 지연 (이 CMSIS는 DWT 타입을 노출하지 않으므로 레지스터 직접 접근)
*********************************************************************************************************
*/

extern uint32_t SystemCoreClock;                 /* system_stm32f4xx.c, HCLK(Hz) */

#define  DWT_CTRL_REG    (*(volatile uint32_t *)0xE0001000u)
#define  DWT_CYCCNT_REG  (*(volatile uint32_t *)0xE0001004u)
#define  SCB_DEMCR_REG   (*(volatile uint32_t *)0xE000EDFCu)

static void LCD_DelayInit(void)
{
    SCB_DEMCR_REG  |= (1u << 24);                 /* TRCENA      */
    DWT_CYCCNT_REG  = 0u;
    DWT_CTRL_REG   |= (1u << 0);                  /* CYCCNTENA   */
}

static void LCD_DelayUs(uint32_t us)
{
    uint32_t start = DWT_CYCCNT_REG;
    uint32_t ticks = us * (SystemCoreClock / 1000000u);
    while ((DWT_CYCCNT_REG - start) < ticks) {
    }
}

static void LCD_DelayMs(uint32_t ms)
{
    while (ms-- > 0u) {
        LCD_DelayUs(1000u);
    }
}

/*
*********************************************************************************************************
* I2C 1바이트 전송 (PCF8574 출력 갱신)
*********************************************************************************************************
*/

static uint8_t LCD_I2C_Wait(uint32_t event)
{
    uint32_t to = 100000u;
    while (I2C_CheckEvent(I2C1, event) == ERROR) {
        if (--to == 0u) {
            return 0u;                           /* 디바이스 없음/오류 시 무한 대기 방지 */
        }
    }
    return 1u;
}

static void LCD_I2C_Write(uint8_t data)
{
    uint32_t to = 100000u;
    while ((I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) == SET) && (--to != 0u)) {
    }

    I2C_GenerateSTART(I2C1, ENABLE);
    if (!LCD_I2C_Wait(I2C_EVENT_MASTER_MODE_SELECT)) { return; }

    I2C_Send7bitAddress(I2C1, LCD_I2C_ADDR_8B, I2C_Direction_Transmitter);
    if (!LCD_I2C_Wait(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) {
        I2C_GenerateSTOP(I2C1, ENABLE);
        return;
    }

    I2C_SendData(I2C1, data);
    if (!LCD_I2C_Wait(I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
        I2C_GenerateSTOP(I2C1, ENABLE);
        return;
    }

    I2C_GenerateSTOP(I2C1, ENABLE);
}

/*
*********************************************************************************************************
* HD44780 4비트 전송
*********************************************************************************************************
*/

static void LCD_PulseEnable(uint8_t data)
{
    LCD_I2C_Write(data | LCD_EN | LcdBacklight);
    LCD_DelayUs(1u);                             /* E 펄스 폭 > 450ns */
    LCD_I2C_Write((data & ~LCD_EN) | LcdBacklight);
    LCD_DelayUs(50u);                            /* 명령 처리 시간 */
}

/* 상위 4비트만 전송 (rs=0 명령, rs=LCD_RS 데이터) */
static void LCD_Write4(uint8_t nibble, uint8_t rs)
{
    LCD_PulseEnable((nibble & 0xF0u) | rs);
}

static void LCD_WriteByte(uint8_t value, uint8_t rs)
{
    LCD_Write4(value & 0xF0u, rs);               /* high nibble */
    LCD_Write4((uint8_t)(value << 4), rs);       /* low nibble  */
}

static void LCD_Cmd(uint8_t cmd)
{
    LCD_WriteByte(cmd, 0u);
}

/*
*********************************************************************************************************
* PUBLIC
*********************************************************************************************************
*/

void LCD_Init(void)
{
    GPIO_InitTypeDef gpio;
    I2C_InitTypeDef  i2c;

    LCD_DelayInit();

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);

    gpio.GPIO_Pin   = GPIO_Pin_6 | GPIO_Pin_7;
    gpio.GPIO_Mode  = GPIO_Mode_AF;
    gpio.GPIO_OType = GPIO_OType_OD;             /* I2C 오픈 드레인 */
    gpio.GPIO_PuPd  = GPIO_PuPd_UP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &gpio);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource6, GPIO_AF_I2C1);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource7, GPIO_AF_I2C1);

    i2c.I2C_ClockSpeed          = 100000u;
    i2c.I2C_Mode                = I2C_Mode_I2C;
    i2c.I2C_DutyCycle           = I2C_DutyCycle_2;
    i2c.I2C_OwnAddress1         = 0x00u;
    i2c.I2C_Ack                 = I2C_Ack_Enable;
    i2c.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_Init(I2C1, &i2c);
    I2C_Cmd(I2C1, ENABLE);

    /* HD44780 4비트 초기화 시퀀스 */
    LCD_DelayMs(50u);                            /* 전원 안정 대기 > 40ms */
    LCD_Write4(0x30u, 0u);  LCD_DelayMs(5u);
    LCD_Write4(0x30u, 0u);  LCD_DelayUs(150u);
    LCD_Write4(0x30u, 0u);  LCD_DelayUs(150u);
    LCD_Write4(0x20u, 0u);  LCD_DelayUs(150u);   /* 4비트 모드 진입 */

    LCD_Cmd(0x28u);                              /* 4비트, 2줄, 5x8 */
    LCD_Cmd(0x08u);                              /* 디스플레이 off */
    LCD_Cmd(0x01u);  LCD_DelayMs(2u);            /* clear */
    LCD_Cmd(0x06u);                              /* entry: 증가, 시프트 없음 */
    LCD_Cmd(0x0Cu);                              /* 디스플레이 on, 커서 off */
}

void LCD_Clear(void)
{
    LCD_Cmd(0x01u);
    LCD_DelayMs(2u);
}

void LCD_Print(uint8_t row, const char *p_str)
{
    uint8_t col = 0u;

    LCD_Cmd((row == 0u) ? 0x80u : 0xC0u);        /* DDRAM 주소: row0=0x80, row1=0xC0 */

    while ((*p_str != '\0') && (col < LCD1602_COLS)) {
        LCD_WriteByte((uint8_t)*p_str, LCD_RS);
        p_str++;
        col++;
    }
}
