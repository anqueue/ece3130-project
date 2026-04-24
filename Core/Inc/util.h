#ifndef UTIL_H
#define UTIL_H

#include "stm32l4xx_hal.h"
#include "stm32l4xx_hal_adc.h"
#include "stm32l4xx_hal_gpio.h"
#include <stdint.h>
#include <stdbool.h>

enum GameState {
    GAME_WELCOME = 0,
    GAME_RUNNING,
    GAME_OVER
};

extern volatile enum GameState GAME_STATE;
extern volatile uint8_t POINTS;

void test();

// Game functions
void g_PrintWelcome();

// Helper functions

// Clear the LCD
void h_ClearLCD();

// Set the LCD line to 0 or 1
void h_SetLine(uint8_t);

// Sets the cursor visibility. 0 for off, 1 for on
void h_SetCursor(uint8_t);

// Get the current resistance
uint16_t h_GetResistance(ADC_HandleTypeDef *hadc1);

bool h_IsPressedSW2();

// Library functions
void Write_String_LCD(char*);
void Write_Char_LCD(uint8_t);
void Write_Instr_LCD(uint8_t);
void LCD_nibble_write(uint8_t, uint8_t);
void Write_SR_LCD(uint8_t);
void LCD_Init(void);
void Write_SR_7S(uint8_t temp_Enable, uint8_t temp_Digit);
void Write_7Seg(uint8_t temp_Enable, uint8_t temp_Digit);
#endif