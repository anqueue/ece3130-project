#include "util.h"
#include "stm32l476xx.h"
#include "stm32l4xx_hal.h"
#include "stm32l4xx_hal_adc.h"
#include "stm32l4xx_hal_gpio.h"
#include <stdint.h>

uint8_t LCD_CURRENT_LINE = 0;      // 0 is top line, 1 is bottom line
uint8_t LCD_CURSOR_VISIBILITY = 0; // 0 is off, 1 is on

uint16_t RESISTORS[5] = {586, 1057, 489, 224, 99};
char *RESISTOR_STRINGS[5] = {"560", "1k", "470", "220", "100"};

volatile enum GameState GAME_STATE = GAME_WELCOME;
volatile uint8_t POINTS = 0;
volatile uint16_t TIME_LEFT_MS = 0;
bool DEV_MODE = false;

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
  char *WELCOME_l2DEV = "DEV MODE ON";

  h_ClearLCD();
  h_SetLine(0);
  Write_String_LCD(WELCOME_l1);
  h_SetLine(1);
  if (DEV_MODE) {
    Write_String_LCD(WELCOME_l2DEV);
  } else {
    Write_String_LCD(WELCOME_l2);
  }
  h_SetLine(0);
}

void g_GetReady() {
  char *GET_READY_l1 = "  Get Ready...  ";
  char *GET_READY_l2 = "                ";

  h_ClearLCD();
  h_SetLine(0);
  Write_String_LCD(GET_READY_l1);
  h_SetLine(1);
  Write_String_LCD(GET_READY_l2);
  h_SetLine(0);
}

void g_ResistorFound() {
  char *FOUND_l1 = "   Good Job!   ";
  char *FOUND_l2 = " You found it! ";

  h_ClearLCD();
  h_SetLine(0);
  Write_String_LCD(FOUND_l1);
  h_SetLine(1);
  Write_String_LCD(FOUND_l2);
  h_SetLine(0);
}

void g_ResistorNotFound() {
  char *LOST_l1 = " Wrong Resistor!";
  char *LOST_l2 = " Next round...";

  h_ClearLCD();
  h_SetLine(0);
  Write_String_LCD(LOST_l1);
  h_SetLine(1);
  Write_String_LCD(LOST_l2);
  h_SetLine(0);
}

void g_CompleteFiveRounds() {
  char *FIVE_l1 = " 5 rounds done! ";
  char *FIVE_l2 = "    Goodbye!    ";

  h_ClearLCD();
  h_SetLine(0);
  Write_String_LCD(FIVE_l1);
  h_SetLine(1);
  Write_String_LCD(FIVE_l2);
  h_SetLine(0);
}

void h_ClearLCD() { Write_Instr_LCD(0x01); }

void h_SetLine(uint8_t line) {
  LCD_CURRENT_LINE = line;
  if (line == 0) {
    Write_Instr_LCD(0x80);
  } else {
    Write_Instr_LCD(0xC0);
  }
}

void h_HomeCursor() { Write_Instr_LCD(0x02); }

void h_SetCursor(uint8_t new) {
  if (new == 0) {
    LCD_CURSOR_VISIBILITY = 0;
    Write_Instr_LCD(0x0C);
  } else {
    LCD_CURSOR_VISIBILITY = 1;
    Write_Instr_LCD(0x0E);
  }
}

uint16_t h_GetResistance(ADC_HandleTypeDef *hadc1) {
  int samples = 5;
  uint32_t sum_adc = 0;
  uint32_t sum_ref = 0;
  ADC_ChannelConfTypeDef sConfig = {0}; // empty config struct

  // https://controllerstech.com/stm32-adc-5-multiple-channels-without-dma/
  sConfig.Rank =
      ADC_REGULAR_RANK_1; // Not sure what rank does but controllerstech sets it
                          // to 1 so we will too
  sConfig.SamplingTime =
      ADC_SAMPLETIME_2CYCLES_5; // shortest sampling time since we average it
                                // ourselves
  sConfig.SingleDiff = ADC_SINGLE_ENDED; // single ended mode - not differential
  sConfig.OffsetNumber =
      ADC_OFFSET_NONE; // offset not needed since ADC resolution is unchanged
  sConfig.Offset = 0;

  // Average multiple samples to reduce noise
  sConfig.Channel = ADC_CHANNEL_5; // PA0 (v_adc) is IN5
  HAL_ADC_ConfigChannel(hadc1, &sConfig);
  for (int i = 0; i < samples; i++) {
    HAL_ADC_Start(hadc1);
    HAL_ADC_PollForConversion(hadc1, HAL_MAX_DELAY);
    sum_adc += HAL_ADC_GetValue(hadc1);
    HAL_ADC_Stop(hadc1);

    HAL_Delay(2); // 10ms delay between samples
  }

  sConfig.Channel = ADC_CHANNEL_9; // PA2 (v_ref) is IN9
  HAL_ADC_ConfigChannel(hadc1, &sConfig);
  for (int i = 0; i < samples; i++) {

    HAL_ADC_Start(hadc1);
    HAL_ADC_PollForConversion(hadc1, HAL_MAX_DELAY);
    sum_ref += HAL_ADC_GetValue(hadc1);
    HAL_ADC_Stop(hadc1);

    HAL_Delay(2); // 10ms delay between samples
  }

  uint16_t v_adc_RAW = sum_adc / samples;
  uint16_t v_ref_RAW = sum_ref / samples;

  // ADC values are from 0-4095 corresponding with 0-3.3V/v_ref
  float V_adc = v_adc_RAW / 4095.0f;
  float V_ref = v_ref_RAW / 4095.0f;

  // 1kOhm, but I measured it to be 986 Ohms with a multimeter
  float R_known = 986;

  // check if we have any bad readings
  if (V_ref <= 0.0f || V_adc <= 0.0f || V_adc >= V_ref) {
    return 0;
  }

  // Voltage divider
  // (3.3V) -> R_unknown -> (V_adc) -> R_known -> GND
  //  V_adc = 3.3V * (R_known / (R_known + R_unknown))
  // Solving for R_unknown gives us:
  float R_unknown = R_known * ((V_ref - V_adc) / V_adc);

  // Convert from float to uint16_t
  return (uint16_t)R_unknown;
}

uint16_t h_GetRandomishValue(ADC_HandleTypeDef *hadc1) {
  HAL_ADC_Start(hadc1);
  HAL_ADC_PollForConversion(hadc1, HAL_MAX_DELAY);
  uint16_t random = HAL_ADC_GetValue(hadc1);
  HAL_ADC_Stop(hadc1);
  return random;
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

  // Load the custom omega symbol into the LCDs CGRAM
  h_LoadOhmSymbol();
}

/*
  Instead of putting the code inside of the stm32l4xx_it.c SysTick_Handler,
  we made this function to be called from there that way its easy to get back to

  The default SysTick time is 1ms, which is perfect because we don't need any
  more or less precision for the game timer
*/
void user_SysTick_Handler() {
  // Check if we are in the middle of a game
  if (GAME_STATE == GAME_GET_READY) {
    // If so, decrement the time by 1ms
    if (TIME_LEFT_MS > 0) {
      TIME_LEFT_MS--;
    } else {
      // get ready time is up
      GAME_STATE = GAME_POST_ROUND;
    }
  } else {
    if (GAME_STATE == GAME_RUNNING) {
      // If so, decrement the time by 1ms
      if (TIME_LEFT_MS > 0) {
        TIME_LEFT_MS--;
      } else {
        // Times up! End the game
        GAME_STATE = GAME_POST_ROUND;
      }
    }
  }
}

void h_Time7Segment(uint16_t time) {
  uint8_t tens = (time / 1000) % 10;
  uint8_t ones = (time / 100) % 10;
  uint8_t tenths = (time / 10) % 10;
  uint8_t hundredths = time % 10;

  Write_7Seg(1, tens, true);
  Write_7Seg(2, ones, false);
  Write_7Seg(3, tenths, false);
  Write_7Seg(4, hundredths, false);
}

void h_Time7SegmentDigit(uint16_t time, uint8_t digit) {
  uint8_t tens = (time / 1000) % 10;
  uint8_t ones = (time / 100) % 10;
  uint8_t tenths = (time / 10) % 10;
  uint8_t hundredths = time % 10;

  switch (digit) {
  case 1:
    Write_7Seg(1, tens, true);
    break;
  case 2:
    Write_7Seg(2, ones, false);
    break;
  case 3:
    Write_7Seg(3, tenths, false);
    break;
  case 4:
    Write_7Seg(4, hundredths, false);
    break;
  }
}

void h_LoadOhmSymbol(void) {
  // Run Set CGRAM Address command
  Write_Instr_LCD(0b01000000);

  // 5x8 bitmap
  // I think this looks pretty good for a 5x8 screen
  uint8_t ohm_pattern[8] = {
      0b00000, // ____
      0b01110, //  XXX
      0b10001, // X   X
      0b10001, // X   X
      0b10001, // X   X
      0b01010, //  X X
      0b11011, // XX XX
      0b00000, // _____
  };

  // write row by row to the lcd
  for (int i = 0; i < 8; i++) {
    Write_Char_LCD(ohm_pattern[i]);
  }

  // Run Set DDRAM address/get back to normal mode
  Write_Instr_LCD(0b10000000);
}

void h_WriteOhmSymbol(void) {
  // Write custom character 0 (the Ohm symbol) to current cursor position
  Write_Char_LCD(0);
}

void h_7S_Scheduled() {
  // Prints only ONE 7S digit, helps with scheduling the 7S and LCD and game
  // logic without blocking

  // we use a static variable to keep track
  // https://www.geeksforgeeks.org/cpp/static-keyword-cpp/
  static uint8_t currentDigit = 0;
  h_Time7SegmentDigit(TIME_LEFT_MS, currentDigit);
  currentDigit++;
  if (currentDigit > 4) {
    currentDigit = 1;
  }
}

void h_7S_Scheduled_Param(uint16_t time) {
  // Prints only ONE 7S digit, helps with scheduling the 7S and LCD and game
  // logic without blocking

  // we use a static variable to keep track
  // https://www.geeksforgeeks.org/cpp/static-keyword-cpp/
  static uint8_t currentDigit = 0;
  h_Time7SegmentDigit(time, currentDigit);
  currentDigit++;
  if (currentDigit > 4) {
    currentDigit = 1;
  }
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
    // HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
    // HAL_Delay(1);
    mask = (mask >> 1);
  }

  mask = 0b10000000;

  for (i = 0; i < 8; i++) {
    if ((temp_Enable & mask) == 0)
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);
    else
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
    // HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
    // HAL_Delay(1);
    mask = mask >> 1;
  }

  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_RESET);
}

void Write_7Seg(uint8_t temp_Enable, uint8_t temp_Digit, bool decimal) {
  uint8_t Enable[5] = {0x00, 0x08, 0x04, 0x02, 0x01};
  uint8_t Digit[10] = {0xC0, 0xF9, 0xA4, 0xB0, 0x99,
                       0x92, 0x82, 0xF8, 0x80, 0x90};
  if (decimal) {
    // its active low, we need to clear the MSB bit to turn on the decimal point
    Digit[temp_Digit] = Digit[temp_Digit] & 0b01111111;
  } else {
    // make sure the decimal point is off
    Digit[temp_Digit] = Digit[temp_Digit] | 0b10000000;
  }
  Write_SR_7S(Enable[temp_Enable], Digit[temp_Digit]);
}