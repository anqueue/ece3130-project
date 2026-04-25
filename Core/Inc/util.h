#ifndef UTIL_H
#define UTIL_H

#include "stm32l4xx_hal.h"
#include "stm32l4xx_hal_adc.h"
#include "stm32l4xx_hal_gpio.h"
#include <stdint.h>
#include <stdbool.h>

/*
    Welcome State: only for the first welcome screen
    Get Ready State: we pick a random resistor and then start a 3s countdown timer...
    -> after the 3s is up, we display the target resistor and switch to the Running state
    Running State: the user must find the resistor within time limit, we constantly check the measured resistance
    and if its within a margin, we go to postround, otherwise, they lose and go back to the welcome screen

*/
enum GameState {
    GAME_WELCOME,
    GAME_GET_READY,
    GAME_RUNNING,
    GAME_POST_ROUND
};

extern volatile enum GameState GAME_STATE;
extern volatile uint8_t POINTS;
extern volatile uint16_t TIME_LEFT_MS;
extern volatile bool SW2_pressed;
extern char* RESISTOR_STRINGS[5];
extern uint16_t RESISTORS[5];
extern bool DEV_MODE;

void test();

// Game functions
void g_PrintWelcome();
void g_GetReady();
void g_ResistorFound();
void g_ResistorNotFound();
void g_CompleteFiveRounds();

void user_SysTick_Handler();
void user_EXTI15_10_IRQHandler();

// Helper functions

// Clear the LCD
void h_ClearLCD();

// Homes the cursor to top left
void h_HomeCursor();

void h_7S_Scheduled();
void h_7S_Scheduled_Param(uint16_t time);
void h_Time7SegmentDigit(uint16_t time, uint8_t digit);


// Set the LCD line to 0 or 1
void h_SetLine(uint8_t);

// Sets the cursor visibility. 0 for off, 1 for on
void h_SetCursor(uint8_t);

// Get the current resistance
uint16_t h_GetResistance(ADC_HandleTypeDef *hadc1);
uint16_t h_GetRandomishValue(ADC_HandleTypeDef *hadc1);

void h_Time7Segment(uint16_t time);

bool h_IsPressedSW2();

// Library functions
void Write_String_LCD(char*);
void Write_Char_LCD(uint8_t);
void Write_Instr_LCD(uint8_t);
void LCD_nibble_write(uint8_t, uint8_t);
void Write_SR_LCD(uint8_t);
void LCD_Init(void);
void Write_SR_7S(uint8_t temp_Enable, uint8_t temp_Digit);
void Write_7Seg(uint8_t temp_Enable, uint8_t temp_Digit, bool decimal);

// Load custom Ohm symbol into CGRAM at position 0
void h_LoadOhmSymbol(void);

// Write custom Ohm symbol to LCD
void h_WriteOhmSymbol(void);
#endif