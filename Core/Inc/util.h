#ifndef UTIL_H
#define UTIL_H

#include "stm32l4xx_hal.h"
#include "stm32l4xx_hal_gpio.h"

void test();
void Write_String_LCD(char*);
void Write_Char_LCD(uint8_t);
void Write_Instr_LCD(uint8_t);
void LCD_nibble_write(uint8_t, uint8_t);
void Write_SR_LCD(uint8_t);
void LCD_Init(void);
void Write_SR_7S(uint8_t temp_Enable, uint8_t temp_Digit);
void Write_7Seg(uint8_t temp_Enable, uint8_t temp_Digit);
#endif