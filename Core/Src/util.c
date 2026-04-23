#include "util.h"

void test() {
  for (int i = 0; i < 440; i++) {
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_SET);

    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_RESET);
    HAL_Delay(1);
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