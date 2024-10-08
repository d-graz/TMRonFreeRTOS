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
#define __DEBUG__
//#define __SABOTAGE__
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "task.h"

// import ascon api headers
#include "api.h"
#include "ascon.h"
#include "crypto_aead.h"
#include "permutations.h"
#include "word.h"
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
UART_HandleTypeDef huart2;

/* Definitions for TheTask */

// Define the task handle
osThreadId_t taskUser;
const osThreadAttr_t taskUserAttributes = {
  .name = "UserTask",
  .stack_size = 1000 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

osThreadId_t taskAscon;
const osThreadAttr_t taskAsconAttributes = {
  .name = "AsconTask",
  .stack_size = 4000 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);

// define the task bodies
void taskUserBody(void *argument);
void taskAsconBody(void *argument);

// define the commit functions
void commitAscon();

// utility function
void init_buffer(unsigned char* buffer, unsigned long long numbytes) {
  unsigned long long i;
  for (i = 0; i < numbytes; i++) buffer[i] = (unsigned char)i;
}
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */

#define AD_LENGTH 16
#define M_LENGTH 64
#define EXEC_STATE_ACTIVE 1
#define EXEC_STATE_INACTIVE 0
#define EXEC_STATE_DONE 2
#define EXEC_MODE_ENCRYPT 1
#define EXEC_MODE_DECRYPT 0

//LIM: cannot use pointers inside this struct
typedef struct inputAscon {
  uint8_t exec_state;
  uint8_t exec_mode;
  unsigned char cypher[M_LENGTH + CRYPTO_ABYTES];
  unsigned long long cypher_len;
  unsigned char message[M_LENGTH];
  unsigned long long message_len;
  unsigned char ad[AD_LENGTH];
  unsigned long long ad_len;
  unsigned char nonce[CRYPTO_NPUBBYTES];
  unsigned char key[CRYPTO_KEYBYTES];
} inputAscon_t;

typedef struct outputAscon {
  uint8_t exec_state;
  uint8_t exec_mode;
  unsigned char cypher[M_LENGTH + CRYPTO_ABYTES];
  unsigned char message[M_LENGTH];
  unsigned long long cypher_len;
  unsigned long long message_len;
} outputAscon_t;

void commitAscon(){
  outputAscon_t * output = (outputAscon_t*) xGetOutput();
  if (output->exec_state == EXEC_STATE_DONE){
    if (output->exec_mode == EXEC_MODE_ENCRYPT){
      printf("OUTPUT Cypher: ");
      for (int i = 0; i < output->cypher_len; i++){
        printf("%02x", output->cypher[i]);
      }
      printf("\n");
    } else if (output->exec_mode == EXEC_MODE_DECRYPT){
      printf("OUTPUT Message: ");
      for (int i = 0; i < output->message_len; i++){
        printf("%02x", output->message[i]);
      }
      printf("\n");
    }
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

  taskAscon = osThreadNewRedundant(taskAsconBody, NULL, &taskAsconAttributes);  // in task.h xTaskCreateRedundant
  taskUser = osThreadNew(taskUserBody, NULL, &taskUserAttributes); 

  // setting the input of the ascon task
  inputAscon_t * inputAscon = pvPortMalloc(sizeof(inputAscon_t));
  inputAscon->exec_state = EXEC_STATE_INACTIVE;
  xSetInput(taskAscon, inputAscon, sizeof(inputAscon_t));

  // setting the output of the ascon task
  xSetOutput(taskAscon, sizeof(outputAscon_t));

  // set the commit function of the ascon task
  xSetCommitFunction(taskAscon, commitAscon, NULL);

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

void taskUserBody(void *argument)
{
  // declarizing and initializing
  //unsigned char cypher[M_LENGTH + CRYPTO_ABYTES];
  unsigned char message[M_LENGTH];
  unsigned char ad[AD_LENGTH];
  unsigned char nonce[CRYPTO_NPUBBYTES];
  unsigned char key[CRYPTO_KEYBYTES];
  init_buffer(message, M_LENGTH);
  init_buffer(ad, AD_LENGTH);
  init_buffer(nonce, CRYPTO_NPUBBYTES);
  init_buffer(key, CRYPTO_KEYBYTES);
  printf("Message: ");
  for (int i = 0; i < M_LENGTH; i++){
    printf("%02x", message[i]);
  }
  printf("\n");

  // await some time for the ascon task to be ready
  printf("\nAwaiting for the encryption service to be ready\n");
  osDelay(1000);
  TaskHandle_t taskAscon = xTaskGetHandle("AsconTask");
  uint8_t ready = 0;
  // wait for the output of the ascon task to be set to inactive
  while (!ready){
    outputAscon_t * output = (outputAscon_t*) xGetTaskOutput(taskAscon);
    if (output->exec_state == EXEC_STATE_INACTIVE){
      ready = 1;
    }
    osDelay(50);
  }

  // preparing the input to the ascon task
  inputAscon_t * input = pvPortMalloc(sizeof(inputAscon_t));
  input->exec_state = EXEC_STATE_ACTIVE;
  input->exec_mode = EXEC_MODE_ENCRYPT;
  memcpy(input->message, message, M_LENGTH);
  input->message_len = M_LENGTH;
  memcpy(input->ad, ad, AD_LENGTH);
  input->ad_len = AD_LENGTH;
  memcpy(input->nonce, nonce, CRYPTO_NPUBBYTES);
  memcpy(input->key, key, CRYPTO_KEYBYTES);
  printf("Input set for encryption\n");
  // setting the input of the ascon task
  xSetTaskInput(taskAscon, input);

  printf("Await for the decryption serivce to be ready\n");
  // waiting for the ascon task to finish
  osDelay(1000);
  ready = 0;
  outputAscon_t * output;
  while (!ready){
    output = (outputAscon_t*) xGetTaskOutput(taskAscon);
    if (output->exec_state == EXEC_STATE_INACTIVE){
      ready = 1;
    }
    osDelay(50);
  }
  input->exec_mode = EXEC_MODE_DECRYPT;
  memcpy(input->cypher, output->cypher, output->cypher_len);
  input->cypher_len = output->cypher_len;
  xSetTaskInput(taskAscon, input);
  printf("Input set for decryption\n");

  printf("Await for the decryption serivce to finish\n");
  // waiting for the ascon task to finish
  osDelay(1000);
  ready = 0;
  while (!ready){
    output = (outputAscon_t*) xGetTaskOutput(taskAscon);
    if (output->exec_state == EXEC_STATE_INACTIVE){
      ready = 1;
    }
    osDelay(50);
  }
  printf("Decryption service finished\n");

  if(memcmp(message, output->message, M_LENGTH) == 0){
    printf("\nEncryption and Decryption successful\n");
  } else {
    printf("\nEncryption and/or Decryption failed\n");
  }

  vTaskDelete(NULL);
}

void taskAsconBody(void *argument){
  inputAscon_t * input = (inputAscon_t*) xGetInput();
  outputAscon_t * output = (outputAscon_t*) xGetOutput();
  for(;;){
    // an external task has to set the input
    if(input->exec_state == EXEC_STATE_ACTIVE){

      if(input->exec_mode == EXEC_MODE_ENCRYPT){
        
        crypto_aead_encrypt(input->cypher, &input->cypher_len, input->message, input->message_len, input->ad, input->ad_len, NULL, input->nonce, input->key);
        input->exec_state = EXEC_STATE_DONE;
        output->exec_state = input->exec_state;
        output->exec_mode = input->exec_mode;
        #ifdef __SABOTAGE__
          // sabotage the cypher
          if(strcmp(pcTaskGetName(NULL), "AsconTask") == 0){
            for (int i = 0; i < input->cypher_len; i++){
              input->cypher[i] = 0;
            }
          }
        #endif

        output->cypher_len = input->cypher_len;
        memcpy(output->cypher, input->cypher, input->cypher_len);
        
      } else if(input->exec_mode == EXEC_MODE_DECRYPT){
        
        crypto_aead_decrypt(input->message, &input->message_len, NULL, input->cypher, input->cypher_len, input->ad, input->ad_len, input->nonce, input->key);
        input->exec_state = EXEC_STATE_DONE;
        output->exec_state = input->exec_state;
        output->exec_mode = input->exec_mode;
        output->message_len = input->message_len;
        memcpy(output->message, input->message, input->message_len);
      
      }

    } else if (input->exec_state == EXEC_STATE_DONE) {
      input->exec_state = EXEC_STATE_INACTIVE;
      output->exec_state = input->exec_state;
    }
    osDelay(50);
  }
}

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
