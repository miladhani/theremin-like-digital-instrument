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
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "vl6180x.h"
#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <math.h>
#include "audio.h"
#include "tim.h"
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

/* USER CODE BEGIN PV */

volatile uint32_t fs_ticks = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

static void I2C_Scan(void)
{
  printf("I2C scan start\r\n");   // start the scan

  // scan all 7-bit I2C addresses (1..127)
  for (uint8_t a = 1; a < 128; a++)
  {
    // We need to shift from 7 to 8 because of HAL
    if (HAL_I2C_IsDeviceReady(&hi2c1, (uint16_t)(a << 1), 1, 20) == HAL_OK)
    {
      // print device 7-bit address
      printf("Found 0x%02X\r\n", a);
    }
  }

  printf("I2C scan end\r\n");     // scan ending
}

// map filtered distance df  -> frequency f
static float map_distance_to_freq(float df)
{
    const float dmin = 40.0f,  dmax = 200.0f;   // distance range. I clamp it between 40 and 200
    const float fmin = 110.0f, fmax = 1760.0f;  // output freq range

    // normalize df to x in [0..1]
    float x = (df - dmin) / (dmax - dmin);
    if (x < 0.0f) x = 0.0f;
    if (x > 1.0f) x = 1.0f;

    // The mapping :
    float ratio = fmax / fmin;
    return fmin * powf(ratio, x);   // exponential. feels better than linear for sound. it smooths the raw number of the distance

}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
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
  MX_I2C1_Init();
  MX_TIM3_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */

  VL6180X_t tof;     // create a VL6180X device object
  // try to init the sensor on I2C1. for checking if the sensor is alive
  if (VL6180X_Init(&tof, &hi2c1, 0x29))
  {
    // successful
    printf("VL6180X init OK\r\n");
  }
  else
  {
    // fail
    printf("VL6180X init FAIL\r\n");
  }

  I2C_Scan();

  const char msg[] = "BOOT is OK\r\n";
  HAL_UART_Transmit(&huart2, (uint8_t*)msg, sizeof(msg)-1, HAL_MAX_DELAY);   // check the BOOT

  // printf("Hello from Polito\r\n");  // printf check

  // same as before. HAL wants 8-bit. so I shift
  uint16_t addr = (0x29 << 1);

  // Finally I can check if the sensor is recognized or not
  HAL_StatusTypeDef ok = HAL_I2C_IsDeviceReady(&hi2c1, addr, 3, 100);    // try 3 times. and each time wait for 100 ms
  if (ok == HAL_OK)
      printf("VL6180X detected\r\n");
  else
      printf("VL6180X NOT detected\r\n");

  // start PWM
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
  // start Fs interrupt timer
  HAL_TIM_Base_Start_IT(&htim2);

  // init audio (ARR is the PWM timer period)
  Audio_Init(&htim3, TIM_CHANNEL_1, (uint16_t)__HAL_TIM_GET_AUTORELOAD(&htim3), 20000);
  Audio_SetWaveform(WAVE_SINE);
  Audio_SetFreqHz(440.0f);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	      // I used a filter to smooth the numbers
	      static float df = 100.0f;
	      const float alpha = 0.15f;    // smoothing factor. it alpha is smaller, the numbers are smoother but it is slower

	      // read distance from sensor
	      uint8_t mm;

	      if (VL6180X_ReadRangeSingle(&tof, &mm, 200))
	      {
	          float d = (float)mm;

	          // sth like saturation
	          if (d < 40)  d = 40;
	          if (d > 200) d = 200;

	          // low-pass filter
	          df = df + alpha * (d - df);

	          // printf("Real = %u  Filtered = %.1f\r\n", mm, df);

	          float f = map_distance_to_freq(df);
	          Audio_SetFreqHz(f);
	          // printf("raw=%u  filt=%.1f  f=%.1f\r\n", mm, df, f);     // this is the main printf to see the values in putty. commented for the last part of the project

	      }
	      HAL_Delay(50); // 20 updates per second

	      static uint32_t last = 0;
	      if (HAL_GetTick() - last >= 1000) {
	          last += 1000;
	          // printf("Fs ticks/sec = %lu\r\n", fs_ticks);   // commented for the last part of the project
	          fs_ticks = 0;
	      }


    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM2)   // Fs timer
  {
    fs_ticks++;
    Audio_Tick();  // ISR
  }
}

// the buttom for changing the waveforms
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == GPIO_PIN_13)
  {
    static uint32_t last = 0;
    uint32_t now = HAL_GetTick();
    if (now - last < 200) return;  // 200 ms debounce
    last = now;

    static uint8_t w = 0;
    w = (w + 1) & 0x03;  // 0..3

    switch (w)
    {
      case 0: Audio_SetWaveform(WAVE_SINE);     break;
      case 1: Audio_SetWaveform(WAVE_TRI);      break;
      case 2: Audio_SetWaveform(WAVE_SQUARE);   break;
      case 3: Audio_SetWaveform(WAVE_SAW);      break;
    }
  }
}


/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
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
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
