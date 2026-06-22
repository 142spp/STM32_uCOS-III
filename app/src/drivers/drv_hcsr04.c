/*
*********************************************************************************************************
* Smart Parking - drv_hcsr04.c
*********************************************************************************************************
*/

#include  "drv_hcsr04.h"
#include  "stm32f4xx.h"
#include  "stm32f4xx_rcc.h"
#include  "stm32f4xx_gpio.h"
#include  "stm32f4xx_tim.h"

void HCSR04_Init(void)
{
    /* TODO: trigger GPIO 출력, echo 핀 타이머 입력 캡처(상승/하강 에지) 설정 */
}

void HCSR04_Trigger(uint8_t sensor_id)
{
    (void)sensor_id;
    /* TODO: 해당 센서의 trigger 핀에 10us 펄스 출력. 센서 간 간섭 방지를 위해 순차 호출. */
}

uint16_t HCSR04_ReadDistance(uint8_t sensor_id)
{
    (void)sensor_id;
    /* TODO: 입력 캡처로 받은 echo 펄스 폭(us)을 거리(mm)로 환산하여 반환.
     *       거리 = echo_us * 0.343 / 2 (mm). 측정 실패 시 HCSR04_DISTANCE_INVALID. */
    return HCSR04_DISTANCE_INVALID;
}
