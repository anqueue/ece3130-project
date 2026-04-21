#include "util.h"
#include "stm32l4xx_hal.h"
#include "stm32l4xx_hal_gpio.h"

void test() {
    for (int i = 0; i < 440; i++) {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_SET);

        HAL_Delay(1);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_RESET);
        HAL_Delay(1);
    }
}