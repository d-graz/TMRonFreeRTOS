/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
//#define __DEBUG__
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "task.h"
#include "semphr.h"
#include "time.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define N_PRIMES 1000
#define REDUNDANT_MODE
#define DELAY 2
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;

/* Definitions for TheTask */

// Define the task handle
osThreadId_t taskBenchmark;
const osThreadAttr_t taskBenchmarkAttributes = {
  .name = "BenchmarkTask",
  .stack_size = 1000 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};

osThreadId_t taskPrime;
const osThreadAttr_t taskPrimeAttributes = {
  .name = "PrimeTask",
  .stack_size = 2000 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
#ifndef REDUNDANT_MODE
  osThreadId_t taskPrime1;
  const osThreadAttr_t taskPrimeAttributes1 = {
    .name = "PrimeTask1",
    .stack_size = 2000 * 4,
    .priority = (osPriority_t) osPriorityNormal,
  };
#endif
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);

// define the task bodies
void taskBenchmarkBody(void *argument);
void taskPrimeBody(void *argument);
#ifndef REDUNDANT_MODE
  void taskPrimeBody1(void *argument);
#endif

// define the commit functions
void commitPrime();
#ifndef REDUNDANT_MODE
  void commitPrime1(uint16_t * arr);
#endif
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */

//create a semaphore to tell the benchmark task that the prime task is done
SemaphoreHandle_t xSemaphore;
#ifndef REDUNDANT_MODE
  SemaphoreHandle_t xSemaphore1;
#endif

// input struct for prime function
typedef struct inputPrime {
  uint16_t n_primes[N_PRIMES];
  uint16_t index;
  uint16_t n;
} inputPrime_t;

void commitPrime(){
  inputPrime_t * input = (inputPrime_t*) xGetInput();
  if (input->index == N_PRIMES){
    printf("\nPrime numbers: ");
    for(int i = 0; i < N_PRIMES; i+=(N_PRIMES/10)){
      for(int j = i; j < i+(N_PRIMES/10); j++){
        printf("%d ", input->n_primes[j]);
      }
      printf("\n");
    }
    xSemaphoreGive(xSemaphore);
  }
}

void commitPrime1(uint16_t * arr){
	for(int i = 0; i < N_PRIMES; i+=(N_PRIMES/10)){
	    for(int j = i; j < i+(N_PRIMES/10); j++){
	      printf("%d ", arr[j]);
	   }
	  printf("\n");
	}
}



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
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of TheTask */
  #ifdef REDUNDANT_MODE
    // create the prime task in redundant mode
    taskPrime = osThreadNewRedundant(taskPrimeBody, NULL, &taskPrimeAttributes);

    // initialize the input
    inputPrime_t * inputPrime = pvPortMalloc(sizeof(inputPrime_t));
    inputPrime->index = 0;
    for(int i = 0; i < N_PRIMES; i++){
      inputPrime->n_primes[i] = 0;
    }
    inputPrime->n = 2;
    xSetInput(taskPrime, inputPrime, sizeof(inputPrime_t));
    xSetCommitFunction(taskPrime, commitPrime, NULL);
  #else
    taskPrime = osThreadNew(taskPrimeBody, NULL, &taskPrimeAttributes);
    taskPrime1 = osThreadNew(taskPrimeBody1, NULL, &taskPrimeAttributes1);
  #endif
  taskBenchmark = osThreadNew(taskBenchmarkBody, NULL, &taskBenchmarkAttributes);

  #ifdef __DEBUG__
  	printTaskList();
  #endif

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
  RCC_OscInitStruct.PLL.PLLDIV = RCC_PLL_DIV3;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

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
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
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
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PC0 PC1 PC2 PC3
                           PC4 PC5 PC6 PC7
                           PC8 PC9 PC10 PC11
                           PC12 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3
                          |GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7
                          |GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11
                          |GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PA0 PA1 PA4 PA6
                           PA7 PA8 PA9 PA10
                           PA11 PA12 PA15 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_4|GPIO_PIN_6
                          |GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10
                          |GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PB0 PB1 PB2 PB10
                           PB11 PB12 PB13 PB14
                           PB15 PB4 PB5 PB6
                           PB7 PB8 PB9 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_10
                          |GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14
                          |GPIO_PIN_15|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6
                          |GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PD2 */
  GPIO_InitStruct.Pin = GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

void taskBenchmarkBody(void *argument)
{
  unsigned long start, end;
  #ifdef REDUNDANT_MODE
    xSemaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(xSemaphore);
    start = xTaskGetTickCount();
    osDelay(10);
    xSemaphoreTake(xSemaphore, portMAX_DELAY);
    end = xTaskGetTickCount();
    printf("Time taken for prime task: %f\n", (double)(end - start)/configTICK_RATE_HZ);
  #else
    xSemaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(xSemaphore);
    xSemaphore1 = xSemaphoreCreateBinary();
    xSemaphoreGive(xSemaphore1);
    start = xTaskGetTickCount();
    osDelay(10);
    xSemaphoreTake(xSemaphore, portMAX_DELAY);
    xSemaphoreTake(xSemaphore1, portMAX_DELAY);
    end = xTaskGetTickCount();
    printf("Time taken for prime task: %f\n", (double)(end - start)/configTICK_RATE_HZ);
  #endif
  vTaskDelete(NULL);
}

void taskPrimeBody(void *argument){
  if(strcmp(pcTaskGetName(NULL), "PrimeTask") == 0){
    xSemaphoreTake(xSemaphore, portMAX_DELAY);
  }
  #ifdef REDUNDANT_MODE
    inputPrime_t * input = (inputPrime_t*) xGetInput();
    while(input->index < N_PRIMES){
      uint8_t is_prime = 1;
      for(int i = 0; i < N_PRIMES && input->n_primes[i] != 0; i++) {
        if(input->n % input->n_primes[i] == 0){
          is_prime = 0;
          break;
        }
      }
      if(is_prime){
        input->n_primes[input->index] = input->n;
        input->index++;
      }
      input->n++;
      osDelay(DELAY);
    }
  #else
    uint16_t n_primes[N_PRIMES];
    uint16_t index = 0;
    uint16_t n = 2;
    for(int i = 0; i < N_PRIMES; i++){
      n_primes[i] = 0;
    }
    while(index < N_PRIMES){
      uint8_t is_prime = 1;
      for(int i = 0; i < N_PRIMES && n_primes[i] != 0; i++) {
        if(n % n_primes[i] == 0){
          is_prime = 0;
          break;
        }
      }
      if(is_prime){
        n_primes[index] = n;
        index++;
      }
      n++;
      osDelay(DELAY);
    }
    xSemaphoreGive(xSemaphore);
  #endif
  vTaskDelete(NULL);
}

#ifndef REDUNDANT_MODE
void taskPrimeBody1(void *argument){
  xSemaphoreTake(xSemaphore1, portMAX_DELAY);
  uint16_t n_primes[N_PRIMES];
    uint16_t index = 0;
    uint16_t n = 2;
    for(int i = 0; i < N_PRIMES; i++){
      n_primes[i] = 0;
    }
    while(index < N_PRIMES){
      uint8_t is_prime = 1;
      for(int i = 0; i < N_PRIMES && n_primes[i] != 0; i++) {
        if(n % n_primes[i] == 0){
          is_prime = 0;
          break;
        }
      }
      if(is_prime){
        n_primes[index] = n;
        index++;
      }
      n++;
      osDelay(DELAY);
    }
    commitPrime1(n_primes);
  xSemaphoreGive(xSemaphore1);
  vTaskDelete(NULL);
}
#endif

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM2 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM2) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

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

#ifdef  USE_FULL_ASSERT
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
