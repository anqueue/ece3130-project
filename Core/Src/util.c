#include "util.h"
#include "stm32l476xx.h"
#include "stm32l4xx_hal_adc.h"
#include "stm32l4xx_hal_gpio.h"

uint8_t LCD_CURRENT_LINE = 0;      // 0 is top line, 1 is bottom line
uint8_t LCD_CURSOR_VISIBILITY = 0; // 0 is off, 1 is on

volatile enum GameState GAME_STATE = GAME_WELCOME;

void test() {
  for (int i = 0; i < 440; i++) {
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_SET);

    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_RESET);
    HAL_Delay(1);
  }
}

void g_PrintWelcome() {
  char *WELCOME_l1 = " Resistor Game";
  char *WELCOME_l2 = " Press to start ";

  h_ClearLCD();
  h_SetLine(0);
  Write_String_LCD(WELCOME_l1);
  h_SetLine(1);
  Write_String_LCD(WELCOME_l2);
  h_SetLine(0);
}

void h_ClearLCD() { Write_Instr_LCD(0x01); }

void h_SetLine(uint8_t line) {
  if (LCD_CURRENT_LINE == 0) {
    LCD_CURRENT_LINE = 1;
    Write_Instr_LCD(0x80);
  } else {
    LCD_CURRENT_LINE = 0;
    Write_Instr_LCD(0xC0);
  }
}

void h_SetCursor(uint8_t new) {
  if (new == 0) {
    LCD_CURSOR_VISIBILITY = 0;
    Write_Instr_LCD(0x0C);
  } else {
    LCD_CURSOR_VISIBILITY = 1;
    Write_Instr_LCD(0x0E);
  }
}

uint16_t h_GetResistance(ADC_HandleTypeDef *hadc) {
  /*
  int KnownR = 980; // 1kOhm
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_SET);

    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
    raw = HAL_ADC_GetValue(&hadc1);

    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_RESET);
    float measured_volts = 3.3f * (raw / 4095.0f);

    float ActiveR = ((3.3 * KnownR) / (measured_volts)) - KnownR;

    int r1000 = ActiveR * 1000;
    int v1000 = measured_volts * 1000;
    sprintf(msg, ">R:%d\r\n>V:%d.%03d\r\n", r1000 / 1000, v1000 / 1000,
            v1000 % 1000);
    HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);

    HAL_Delay(100);
  */

  HAL_ADC_Start(hadc);
  HAL_ADC_PollForConversion(hadc, HAL_MAX_DELAY);
  uint16_t raw = HAL_ADC_GetValue(hadc);
  float measured_volts = 3.3f * (raw / 4095.0f);
  float ActiveR = ((3.3 * 980) / (measured_volts)) - 980;

  return (uint16_t)ActiveR;
}

bool h_IsPressedSW2() {
  if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_11) == GPIO_PIN_SET) {
    return true;
  } else {
    return false;
  }
}

void LCD_Init() {

  HAL_Delay(20);
  LCD_nibble_write(0x30, 0);
  HAL_Delay(5);
  LCD_nibble_write(0x30, 0);
  HAL_Delay(1);
  LCD_nibble_write(0x30, 0);
  HAL_Delay(1);
  LCD_nibble_write(0x20, 0);
  HAL_Delay(1);

  Write_Instr_LCD(0x28);
  Write_Instr_LCD(0x0E);
  Write_Instr_LCD(0x01);
  Write_Instr_LCD(0x06);
}

void LCD_nibble_write(uint8_t temp, uint8_t s) {
  if (s == 0) {
    temp = temp & 0xF0;
    temp = temp | 0x02;
    Write_SR_LCD(temp);

    temp = temp & 0xFD;
    Write_SR_LCD(temp);
  }

  else if (s == 1) {
    temp = temp & 0xF0;
    temp = temp | 0x03;
    Write_SR_LCD(temp);

    temp = temp & 0xFD;
    Write_SR_LCD(temp);
  }
}

void Write_String_LCD(char *temp) {
  int i = 0;

  while (temp[i] != 0) {
    Write_Char_LCD(temp[i]);
    i = i + 1;
  }
}

void Write_Instr_LCD(uint8_t code) {
  LCD_nibble_write(code & 0xF0, 0);

  code = code << 4;
  LCD_nibble_write(code, 0);
}

void Write_Char_LCD(uint8_t code) {
  LCD_nibble_write(code & 0xF0, 1);
  code = code << 4;
  LCD_nibble_write(code, 1);
}

void Write_SR_LCD(uint8_t temp) {
  int i;
  uint8_t mask = 0b10000000;

  for (i = 0; i < 8; i++) {
    if ((temp & mask) == 0)
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);
    else
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
    HAL_Delay(1);

    mask = mask >> 1;
  }

  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_RESET);
}

void Write_SR_7S(uint8_t temp_Enable, uint8_t temp_Digit) {
  int i;
  uint8_t mask = 0b10000000;
  for (i = 0; i < 8; i++) {
    if ((temp_Digit & mask) == 0) {
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);
    } else {
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);
    }
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
    HAL_Delay(1);
    mask = (mask >> 1);
  }

  mask = 0b10000000;

  for (i = 0; i < 8; i++) {
    if ((temp_Enable & mask) == 0)
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);
    else
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
    HAL_Delay(1);
    mask = mask >> 1;
  }

  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_RESET);
}

void Write_7Seg(uint8_t temp_Enable, uint8_t temp_Digit) {
  uint8_t Enable[5] = {0x00, 0x08, 0x04, 0x02, 0x01};
  uint8_t Digit[10] = {0xC0, 0xF9, 0xA4, 0xB0, 0x99,
                       0x92, 0x82, 0xF8, 0x80, 0x90};
  Write_SR_7S(Enable[temp_Enable], Digit[temp_Digit]);
}