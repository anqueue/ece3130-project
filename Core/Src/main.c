/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
#include "stm32l4xx_hal.h"
#include "util.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_ADC1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {

  /* USER CODE BEGIN 1 */
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick.
   */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */
  LCD_Init();
  h_SetCursor(0); // turn off cursor for a cleaner look

welcome:
    // Set the game state to welcome so the interrupt knows where we are
  GAME_STATE = GAME_WELCOME;
  g_PrintWelcome(); // Print the welcome screen text
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  // Currently we can guarentee that game state is on the welcome page.
  // We want to wait until any key is pressed to start the game

  TIME_LEFT_MS = DEV_MODE ? 1000 : 5000;

  // SW2_pressed gets modifed from the EXTI interrupt when the button is pressed.
  // We set it to false here and just wait until its pressed, then immediately set it back to false so we can use it for the rest of the game
  SW2_pressed = false;
  while (!SW2_pressed) {
    // Wait for the initial Press SW2 to start
    h_7S_Scheduled();
  }
  SW2_pressed = false;

  // We base the seed off of the current tick, which is a little random
  uint32_t seed = HAL_GetTick();
  // but then we XOR it with whatever we read from the ADC, which has random
  // noise
  seed ^= h_GetRandomishValue(&hadc1);
  srand(seed);

  uint8_t rounds = 1; // set starting rounds to 1

next_round:
  TIME_LEFT_MS = DEV_MODE ? 1000 : 5000; // get ready is 1s in dev, 5s otherwise

  // Get ready, Start the 5s countdown
  // The systick handler will check state and decrement for us, then transition
  // the state to GAME_RUNNING

  g_GetReady(); // print the get ready screen
  GAME_STATE = GAME_GET_READY;
  while (GAME_STATE == GAME_GET_READY) {
    // Print the countdown on the 7 segment
    h_7S_Scheduled();
  }

  // Freeze the countdown while we prepare the new round display.
  GAME_STATE = GAME_POST_ROUND;

  h_ClearLCD();

  // Set time to 10s in case it didnt get handled in SysTick
  TIME_LEFT_MS = 10000;

  // Pick a random resistor
  // If we add more resistors, this would need to be modified
  size_t target_index = rand() % 5; // Random index from 0 to 4

  // Two buffers to store the lines we print to lcd
  char line1_buffer[16];
  char line2_buffer[16];

  h_SetLine(0);

  // We use stdio to help format the string with the resistor value
  snprintf(line1_buffer, sizeof(line1_buffer), "Find: %s",
           RESISTOR_STRINGS[target_index]);

  Write_String_LCD(line1_buffer);

  // The ohm symbol is defined as a custom character during LCD init

  h_WriteOhmSymbol();
  h_SetLine(1);

  snprintf(line2_buffer, sizeof(line2_buffer), "Round %u/5", rounds);
  Write_String_LCD(line2_buffer);

  // Start the game!
  GAME_STATE = GAME_RUNNING;
  // h_ClearLCD();
  // h_HomeCursor();
  // h_SetLine(1);

  uint16_t resistance = 0;

  // We store the current HAL tick here so we know how often to sample ADC
  // since it takes a decent bit of time, and doing it every loop iteration
  // would reuslt in LCD flickering
  uint32_t lastSample = HAL_GetTick();

  // +/- 10% tolerance with the resistors, seems to work correctly
  float resistorLowTolerance =
      RESISTORS[target_index] - (RESISTORS[target_index] * 0.10f);
  float resistorHighTolerance =
      RESISTORS[target_index] + (RESISTORS[target_index] * 0.10f);

  while (GAME_STATE == GAME_RUNNING) {
    // Go through and print all of the 7 segment values with the global time remaning
    h_7S_Scheduled();
    // h_HomeCursor();
    // h_SetLine(1);

    // only sample ADC every 500ms so we dont spend too much time not updating the 7seg
    if (HAL_GetTick() - lastSample > 500) {
      lastSample = HAL_GetTick();
      resistance = h_GetResistance(&hadc1);
    }

    // We wanted to display the currnet measured resistance during use but that ended up
    // causing too much time between each loop, resulting in 7seg flickering

    // char buffer[16];
    // snprintf(buffer, sizeof(buffer), "%u ", resistance);
    // Write_String_LCD(buffer);
    // h_WriteOhmSymbol();

    // Check if our resistor is correct
    if (resistorLowTolerance < resistance &&
        resistance < resistorHighTolerance) {
      break;
    }
  }

  // SysTick will get us out of the previous while loop after 10 seconds, or when we manually break

  // Store the current time so we can keep displaying it on the 7seg so users can keep track of their PB
  // We also store the current tick so we can use it and break after 2000ms
  uint16_t currTime = TIME_LEFT_MS;
  uint32_t initialTick = HAL_GetTick();
  GAME_STATE = GAME_POST_ROUND;
  while (1) {
    h_7S_Scheduled_Param(currTime);
    if (HAL_GetTick() > (initialTick + 2000)) {
      break;
    }
  }

  // Can be a redudant check if they already slotted in the correct resistor, but its neglibible
  // Check if the resistor found is correct or ont
  if (resistorLowTolerance < resistance && resistance < resistorHighTolerance) {
      // Print the resistor found screen
    g_ResistorFound();

    // Wait a few seconds then move on to the next round
    HAL_Delay(3000);
    rounds++;
    if (rounds > 5) {
        // If they played all 5 rounds, then thats all of the game
        // We jump to welcome instead of next round so someone else can play it
      g_CompleteFiveRounds();
      HAL_Delay(1000);

      // Using goto lets us keep all the game logic in one place without having to reinitialize
      // and reset a bunch of variables. For a larger project this would be a bad practice but this game
      // is pretty linear
      goto welcome;
    } else {
        // Go back to next round for another round
      goto next_round;
    }
  } else {
      // Similarly, show resistor not found screen
    g_ResistorNotFound();
    HAL_Delay(3000);
    rounds++;
    if (rounds > 5) {
      g_CompleteFiveRounds();
      HAL_Delay(1000);
      goto welcome;
    } else {
      goto next_round;
    }
  }

  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */
}
/* USER CODE END 3 */

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
   */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK) {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 10;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK) {
    Error_Handler();
  }
}

/**
 * @brief ADC1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_ADC1_Init(void) {

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_MultiModeTypeDef multimode = {0};
  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
   */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc1.Init.LowPowerAutoWait = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc1.Init.OversamplingMode = DISABLE;
  if (HAL_ADC_Init(&hadc1) != HAL_OK) {
    Error_Handler();
  }

  /** Configure the ADC multi-mode
   */
  multimode.Mode = ADC_MODE_INDEPENDENT;
  if (HAL_ADCEx_MultiModeConfigChannel(&hadc1, &multimode) != HAL_OK) {
    Error_Handler();
  }

  /** Configure Regular Channel
   */
  sConfig.Channel = ADC_CHANNEL_5;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_2CYCLES_5;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */
}

/**
 * @brief USART2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART2_UART_Init(void) {

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK) {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */
}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1 | LD2_Pin | GPIO_PIN_9 | GPIO_PIN_10,
                    GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9 | GPIO_PIN_10, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PA1 LD2_Pin PA9 PA10 */
  GPIO_InitStruct.Pin = GPIO_PIN_1 | LD2_Pin | GPIO_PIN_9 | GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PB10 */
  GPIO_InitStruct.Pin = GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PB11 */
  GPIO_InitStruct.Pin = GPIO_PIN_11;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PC9 PC10 */
  GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PB5 */
  GPIO_InitStruct.Pin = GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 2, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1) {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line) {
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line
     number, ex: printf("Wrong parameters value: file %s on line %d\r\n", file,
     line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
