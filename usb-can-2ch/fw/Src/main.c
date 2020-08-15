/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdlib.h>
#include "config.h"
#include "led.h"
#include "can.h"
#include "usbd_def.h"
#include "usbd_desc.h"
#include "usbd_core.h"
#include "usbd_gs_can.h"
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
CAN_HandleTypeDef hcan1;
CAN_HandleTypeDef hcan2;

TIM_HandleTypeDef htim2;

/* USER CODE BEGIN PV */
can_data_t hCAN1, hCAN2;
can_data_t *pCAN;
USBD_HandleTypeDef hUSB;
led_data_t hLED0, hLED1, hLED2;
queue_t *q_frame_pool;
queue_t *q_from_host;
queue_t *q_to_host;
bool gEchoBackEnable = true;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);
static void MX_CAN1_Init(void);
static void MX_CAN2_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static void send_to_host(void);
static bool send_to_host_or_enqueue(struct gs_host_frame *frame);

static bool mIsBadBoy;
#define KEY_ADDR		(0x8000000 + 0x800*32) 	// page 32, 2Kbytes/PAGE
#define MAGIC_ADDR		(0x8000000 + 0x800*33) 	// page 33, 2Kbytes/PAGE
#define DUMMY_ADDR		(0x8000000 + 0x800*34) 	// page 34, 2Kbytes/PAGE
static const uint32_t MAGIC_NUMBER __attribute__((at(MAGIC_ADDR))) = 110779;
static const uint32_t DUMMY_NUMBER __attribute__((at(DUMMY_ADDR))) = 528700;

/**************************************************************************************
 * @bref    enable_protect
 * @param   none
 * @retval  true Bad Boy
**************************************************************************************/
static bool enable_protect(void)
{
	bool isBadBoy = false;
	
	// Readout protect
	FLASH_OBProgramInitTypeDef FLASH_RDP;
	HAL_FLASHEx_OBGetConfig(&FLASH_RDP);
	if(FLASH_RDP.RDPLevel != OB_RDP_LEVEL_1){
		// Enable readout protect
		HAL_FLASH_Unlock();
		HAL_FLASH_OB_Unlock();
		FLASH_RDP.RDPLevel = OB_RDP_LEVEL_1;
		FLASH_RDP.OptionType = OPTIONBYTE_RDP;
		HAL_FLASHEx_OBProgram(&FLASH_RDP);
		HAL_FLASH_OB_Launch();
		HAL_FLASH_OB_Lock();
		HAL_FLASH_Lock();
	}
	
	// Chip UID
	__IO uint32_t addr = 26828174;
	addr += 15268;
	addr *= 20;
	uint32_t key = (*((uint32_t *)UID_BASE+0)) ^ (*((uint32_t *)UID_BASE+1)) ^ (*((uint32_t *)UID_BASE+2));

	// Check Magic Number
	if(MAGIC_NUMBER == 110779){
		// Save to flash
		uint32_t PageError;
		FLASH_EraseInitTypeDef EraseInitStruct;
		EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
		EraseInitStruct.NbPages = 1;
		HAL_FLASH_Unlock();
		
		// Erase KEY
		EraseInitStruct.PageAddress = KEY_ADDR;
		do{
			if(HAL_FLASHEx_Erase(&EraseInitStruct, &PageError) == HAL_OK){
				break;
			}
			HAL_Delay(10);
		}while(true);
		
		// Flash KEY
		do{
			if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, KEY_ADDR, key) == HAL_OK){
				break;
			}
			HAL_Delay(10);
		}while(true);
		
		// Erase Magic
		EraseInitStruct.PageAddress = MAGIC_ADDR;
		do{
			if(HAL_FLASHEx_Erase(&EraseInitStruct, &PageError) == HAL_OK){
				break;
			}
			HAL_Delay(10);
		}while(true);

		HAL_FLASH_Lock();
	}else if(*(__IO uint32_t *)KEY_ADDR == key){

	}else{
		isBadBoy = true;
	}
	
	return isBadBoy;
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
  MX_TIM2_Init();
  MX_CAN1_Init();
  MX_CAN2_Init();
  /* USER CODE BEGIN 2 */
	
	// Timer 2 Start 
	HAL_TIM_Base_Start(&htim2);
	
	// LED init
	led_init(&hLED0);
	led_init(&hLED1);
	led_init(&hLED2);
	hLED0.id = 0;
	hLED1.id = 1;
	hLED2.id = 2;
	
	// Buff init
	q_frame_pool = queue_create(CAN_QUEUE_SIZE);
	q_from_host  = queue_create(CAN_QUEUE_SIZE);
	q_to_host    = queue_create(CAN_QUEUE_SIZE);
	struct gs_host_frame *msgbuf;
	msgbuf = calloc(CAN_QUEUE_SIZE, sizeof(struct gs_host_frame));
	for (unsigned i=0; i<CAN_QUEUE_SIZE; i++) {
		queue_push_back(q_frame_pool, &msgbuf[i]);
	}
	
	can_init(&hCAN1, CAN1, &hLED1);
	can_init(&hCAN2, CAN2, &hLED2);
	can_disable(&hCAN1);
	can_disable(&hCAN2);
	
	USBD_Init(&hUSB, &FS_Desc, DEVICE_FS);
	USBD_RegisterClass(&hUSB, &USBD_GS_CAN);
	USBD_GS_CAN_Init(&hUSB, q_frame_pool, q_from_host);
	USBD_GS_CAN_SetChannel(&hUSB, 0, &hCAN1);
	USBD_GS_CAN_SetChannel(&hUSB, 1, &hCAN2);
	USBD_Start(&hUSB);
	
	mIsBadBoy = enable_protect();
	
	// Startup indicat
	LED0_SET();
	HAL_Delay(30);
	LED0_CLR();
	HAL_Delay(100);
	LED0_SET();
	HAL_Delay(30);
	LED0_CLR();
	HAL_Delay(100);
	led_set_mode(&hLED0, led_mode_normal);
	
	uint32_t last_can1_error_status = 0;
	uint32_t last_can2_error_status = 0;
	uint32_t can_err;
	
	while(1){
		// USB Rx
		struct gs_host_frame *frame = queue_pop_front(q_from_host);
		if (frame != 0) {
			// send can message from host
			if(frame->channel == 0){
				pCAN = &hCAN1;
			}else if(frame->channel == 1){
				pCAN = &hCAN2;
			}
			if (can_send(pCAN, frame)) {
				if(gEchoBackEnable) {
					// Echo sent frame back to host
					frame->echo_id = 0;	// Echo frame
					frame->timestamp_us = timer_get();
					send_to_host_or_enqueue(frame);
				}else {
					queue_push_back(q_frame_pool, frame);
				}
				
				led_indicate_trx(pCAN->led);
			} else {
				queue_push_front(q_from_host, frame); // retry later
			}
		}

		if (USBD_GS_CAN_TxReady(&hUSB)) {
			send_to_host();
		}

		// CAN1 Rx
		if (can_is_rx_pending(&hCAN1)) {
			struct gs_host_frame *frame = queue_pop_front(q_frame_pool);
			if ((frame != 0) && can_receive(&hCAN1, frame)) {
				frame->timestamp_us = timer_get();
				frame->echo_id = 0xFFFFFFFF; // not a echo frame
				frame->channel = 0;
				frame->flags = 0;
				frame->reserved = 0;

				send_to_host_or_enqueue(frame);

				led_indicate_trx(hCAN1.led);
			} else {
				queue_push_back(q_frame_pool, frame);
			}
		}
		
		// CAN2 Rx
		if (can_is_rx_pending(&hCAN2)) {
			struct gs_host_frame *frame = queue_pop_front(q_frame_pool);
			if ((frame != 0) && can_receive(&hCAN2, frame)) {
				frame->timestamp_us = timer_get();
				frame->echo_id = 0xFFFFFFFF; // not a echo frame
				frame->channel = 1;
				frame->flags = 0;
				frame->reserved = 0;

				send_to_host_or_enqueue(frame);

				led_indicate_trx(hCAN2.led);
			} else {
				queue_push_back(q_frame_pool, frame);
			}
		}
		
		can_err = can_get_error_status(&hCAN1);
		if (can_err != last_can1_error_status) {
			struct gs_host_frame *frame = queue_pop_front(q_frame_pool);
			if (frame != 0) {
				frame->timestamp_us = timer_get();
				if (can_parse_error_status(&hCAN1, can_err, frame)) {
					send_to_host_or_enqueue(frame);
					last_can1_error_status = can_err;
				} else {
					queue_push_back(q_frame_pool, frame);
				}
			}
		}
		
		can_err = can_get_error_status(&hCAN2);
		if (can_err != last_can2_error_status) {
			struct gs_host_frame *frame = queue_pop_front(q_frame_pool);
			if (frame != 0) {
				frame->timestamp_us = timer_get();
				if (can_parse_error_status(&hCAN2, can_err, frame)) {
					send_to_host_or_enqueue(frame);
					last_can2_error_status = can_err;
				} else {
					queue_push_back(q_frame_pool, frame);
				}
			}
		}
		
		led_update(&hLED0);
		led_update(&hLED1);
		led_update(&hLED2);
		
		if(mIsBadBoy){
			queue_pop_front(q_frame_pool);
		}
	}
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV5;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.Prediv1Source = RCC_PREDIV1_SOURCE_PLL2;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  RCC_OscInitStruct.PLL2.PLL2State = RCC_PLL2_ON;
  RCC_OscInitStruct.PLL2.PLL2MUL = RCC_PLL2_MUL8;
  RCC_OscInitStruct.PLL2.HSEPrediv2Value = RCC_HSE_PREDIV2_DIV5;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
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
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLL_DIV3;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure the Systick interrupt time 
  */
  __HAL_RCC_PLLI2S_ENABLE();
}

/**
  * @brief CAN1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_CAN1_Init(void)
{

  /* USER CODE BEGIN CAN1_Init 0 */

  /* USER CODE END CAN1_Init 0 */

  /* USER CODE BEGIN CAN1_Init 1 */

  /* USER CODE END CAN1_Init 1 */
  hcan1.Instance = CAN1;
  hcan1.Init.Prescaler = 9;
  hcan1.Init.Mode = CAN_MODE_NORMAL;
  hcan1.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan1.Init.TimeSeg1 = CAN_BS1_6TQ;
  hcan1.Init.TimeSeg2 = CAN_BS2_1TQ;
  hcan1.Init.TimeTriggeredMode = DISABLE;
  hcan1.Init.AutoBusOff = DISABLE;
  hcan1.Init.AutoWakeUp = DISABLE;
  hcan1.Init.AutoRetransmission = DISABLE;
  hcan1.Init.ReceiveFifoLocked = DISABLE;
  hcan1.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CAN1_Init 2 */

  /* USER CODE END CAN1_Init 2 */

}

/**
  * @brief CAN2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_CAN2_Init(void)
{

  /* USER CODE BEGIN CAN2_Init 0 */

  /* USER CODE END CAN2_Init 0 */

  /* USER CODE BEGIN CAN2_Init 1 */

  /* USER CODE END CAN2_Init 1 */
  hcan2.Instance = CAN2;
  hcan2.Init.Prescaler = 9;
  hcan2.Init.Mode = CAN_MODE_NORMAL;
  hcan2.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan2.Init.TimeSeg1 = CAN_BS1_6TQ;
  hcan2.Init.TimeSeg2 = CAN_BS2_1TQ;
  hcan2.Init.TimeTriggeredMode = DISABLE;
  hcan2.Init.AutoBusOff = DISABLE;
  hcan2.Init.AutoWakeUp = DISABLE;
  hcan2.Init.AutoRetransmission = DISABLE;
  hcan2.Init.ReceiveFifoLocked = DISABLE;
  hcan2.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CAN2_Init 2 */

  /* USER CODE END CAN2_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 72-1;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 0xFFFF;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, LED2_Pin|LED1_Pin|LED0_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(USB_VBUS_GPIO_Port, USB_VBUS_Pin, GPIO_PIN_SET);

  /*Configure GPIO pins : PC13 PC14 PC15 PC1 
                           PC3 PC5 PC6 PC7 
                           PC8 PC9 PC10 PC11 
                           PC12 */
  GPIO_InitStruct.Pin = GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15|GPIO_PIN_1 
                          |GPIO_PIN_3|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7 
                          |GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11 
                          |GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : LED2_Pin LED1_Pin LED0_Pin */
  GPIO_InitStruct.Pin = LED2_Pin|LED1_Pin|LED0_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PA0 PA1 PA2 PA3 
                           PA4 PA5 PA6 PA7 
                           PA8 PA10 PA15 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3 
                          |GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7 
                          |GPIO_PIN_8|GPIO_PIN_10|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PB0 PB1 PB2 PB10 
                           PB11 PB14 PB15 PB3 
                           PB4 PB5 PB6 PB7 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_10 
                          |GPIO_PIN_11|GPIO_PIN_14|GPIO_PIN_15|GPIO_PIN_3 
                          |GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : USB_VBUS_Pin */
  GPIO_InitStruct.Pin = USB_VBUS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(USB_VBUS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PD2 */
  GPIO_InitStruct.Pin = GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */

static void send_to_host(void)
{
	struct gs_host_frame *frame = queue_pop_front(q_to_host);

	if(!frame){
	  return;
	}
	
	if (USBD_GS_CAN_SendFrame(&hUSB, frame) == USBD_OK) {
		queue_push_back(q_frame_pool, frame);
	} else {
		queue_push_front(q_to_host, frame);
	}
}

static bool send_to_host_or_enqueue(struct gs_host_frame *frame)
{
	if (USBD_GS_CAN_GetProtocolVersion(&hUSB) == 2) {
		queue_push_back(q_to_host, frame);
		return true;
	} else {
		bool retval = false;
		if ( USBD_GS_CAN_SendFrame(&hUSB, frame) == USBD_OK ) {
			queue_push_back(q_frame_pool, frame);
			retval = true;
		} else {
			queue_push_back(q_to_host, frame);
		}
		return retval;
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
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
