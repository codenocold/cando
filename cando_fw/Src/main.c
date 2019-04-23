/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdlib.h>
#include "config.h"
#include "flash.h"
#include "led.h"
#include "can.h"
#include "dfu.h"
#include "queue.h"
#include "usbd_def.h"
#include "usbd_desc.h"
#include "usbd_core.h"
#include "usbd_gs_can.h"

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
led_data_t hLED;
can_data_t hCAN;
USBD_HandleTypeDef hUSB;
CAN_HandleTypeDef hcan;
TIM_HandleTypeDef htim2;
queue_t *q_frame_pool;
queue_t *q_from_host;
queue_t *q_to_host;
uint32_t received_count = 0;

/* Private function prototypes -----------------------------------------------*/
static bool send_to_host_or_enqueue(struct gs_host_frame *frame);
static void send_to_host(void);
static void enable_readout_protect(void);
static bool is_bad_boy(void);
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
	uint32_t tick = 0;
	uint32_t last_can_error_status = 0;
	
	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();
	
	/* Configure the system clock */
	SystemClock_Config();
	
	enable_readout_protect();
	
	MX_GPIO_Init();
	MX_TIM2_Init();
    HAL_TIM_Base_Start(&htim2);

	flash_load();
	led_init(&hLED);
	led_set_mode(&hLED, led_mode_off);
	
	can_init(&hCAN, CAN);
	can_disable(&hCAN);
	
	q_frame_pool = queue_create(CAN_QUEUE_SIZE);
	q_from_host  = queue_create(CAN_QUEUE_SIZE);
	q_to_host    = queue_create(CAN_QUEUE_SIZE);

	struct gs_host_frame *msgbuf = calloc(CAN_QUEUE_SIZE, sizeof(struct gs_host_frame));
	for (unsigned i=0; i<CAN_QUEUE_SIZE; i++) {
		queue_push_back(q_frame_pool, &msgbuf[i]);
	}
	
	USBD_Init(&hUSB, &FS_Desc, DEVICE_FS);
	USBD_RegisterClass(&hUSB, &USBD_GS_CAN);
	USBD_GS_CAN_Init(&hUSB, q_frame_pool, q_from_host, &hLED);
	USBD_GS_CAN_SetChannel(&hUSB, 0, &hCAN);
	USBD_Start(&hUSB);

	HAL_GPIO_WritePin(CAN_SILENT_GPIO_Port, CAN_SILENT_Pin, GPIO_PIN_RESET);
	
	for(tick=0; tick<2; tick++){
		LED_SET();
		HAL_Delay(30);
		LED_CLR();
		HAL_Delay(100);
	}
	
	while(1){
		struct gs_host_frame *frame = queue_pop_front(q_from_host);
		if (frame != 0) { // send can message from host
			if (can_send(&hCAN, frame)) {
				// Echo sent frame back to host
				frame->timestamp_us = timer_get();
				send_to_host_or_enqueue(frame);
				
				led_indicate_trx(&hLED);
			} else {
				queue_push_front(q_from_host, frame); // retry later
			}
		}

		if (USBD_GS_CAN_TxReady(&hUSB)) {
			send_to_host();
		}

		if (can_is_rx_pending(&hCAN)) {
			struct gs_host_frame *frame = queue_pop_front(q_frame_pool);
			if ((frame != 0) && can_receive(&hCAN, frame)) {
             			received_count++;

				frame->timestamp_us = timer_get();
				frame->echo_id = 0xFFFFFFFF; // not a echo frame
				frame->channel = 0;
				frame->flags = 0;
				frame->reserved = 0;

				send_to_host_or_enqueue(frame);

				led_indicate_trx(&hLED);

			} else {
				queue_push_back(q_frame_pool, frame);
			}

		}

		uint32_t can_err = can_get_error_status(&hCAN);
		if (can_err != last_can_error_status) {
			struct gs_host_frame *frame = queue_pop_front(q_frame_pool);
			if (frame != 0) {
				frame->timestamp_us = timer_get();
				if (can_parse_error_status(can_err, frame)) {
					send_to_host_or_enqueue(frame);
					last_can_error_status = can_err;
				} else {
					queue_push_back(q_frame_pool, frame);
				}
			}
		}

		led_update(&hLED);
		
		if(HAL_GetTick()-tick > 1000){
			tick = HAL_GetTick();
			if(is_bad_boy()){
				queue_pop_front(q_frame_pool);
				queue_pop_front(q_from_host);
				queue_pop_front(q_to_host);
			}
		}

//		if (USBD_GS_CAN_DfuDetachRequested(&hUSB)) {
//			dfu_run_bootloader();
//		}
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

static void enable_readout_protect(void)
{
	FLASH_OBProgramInitTypeDef OptionsBytesStruct;
	
	/* Unlock the Flash to enable the flash control register access *************/ 
	HAL_FLASH_Unlock();

	/* Unlock the Options Bytes *************************************************/
	HAL_FLASH_OB_Unlock();

	/* Get pages write protection status ****************************************/
	HAL_FLASHEx_OBGetConfig(&OptionsBytesStruct);
	
	if(OptionsBytesStruct.RDPLevel == OB_RDP_LEVEL_0){
		// Enable readout protect
		OptionsBytesStruct.RDPLevel = OB_RDP_LEVEL_1;
		HAL_FLASHEx_OBProgram(&OptionsBytesStruct);
		
		// Programe key
		uint32_t error;
		FLASH_EraseInitTypeDef erase_pages;
		erase_pages.PageAddress = 0x0801F000;	// Page 62, Page size 2Kbytes
		erase_pages.NbPages = 1;
		erase_pages.TypeErase = FLASH_TYPEERASE_PAGES;
		__HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_WRPERR | FLASH_SR_PGERR);
		HAL_FLASHEx_Erase(&erase_pages, &error);
		if (error==0xFFFFFFFF) { // erase finished successfully
			HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, 0x0801F000, get_uid());
		}
		
		/* Generate System Reset to load the new option byte values ***************/
		HAL_FLASH_OB_Launch();
	}
	
	/* Lock the Options Bytes *************************************************/
	HAL_FLASH_OB_Lock();
	
	/* Lock the Flash to disable the flash control register access (recommended
     to protect the FLASH memory against possible unwanted operation) *********/
	HAL_FLASH_Lock();
}

static bool is_bad_boy(void)
{
	if(get_uid() == (*(uint32_t*)(0x0801F000))){
		return false;
	}
	
	return true;
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
  RCC_CRSInitTypeDef RCC_CRSInitStruct = {0};

  /**Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /**Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI48;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_HSI48;

  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
  /**Enable the SYSCFG APB clock 
  */
  __HAL_RCC_CRS_CLK_ENABLE();
  /**Configures CRS 
  */
  RCC_CRSInitStruct.Prescaler = RCC_CRS_SYNC_DIV1;
  RCC_CRSInitStruct.Source = RCC_CRS_SYNC_SOURCE_USB;
  RCC_CRSInitStruct.Polarity = RCC_CRS_SYNC_POLARITY_RISING;
  RCC_CRSInitStruct.ReloadValue = __HAL_RCC_CRS_RELOADVALUE_CALCULATE(48000000,1000);
  RCC_CRSInitStruct.ErrorLimitValue = 34;
  RCC_CRSInitStruct.HSI48CalibrationValue = 32;

  HAL_RCCEx_CRSConfig(&RCC_CRSInitStruct);
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
  htim2.Init.Prescaler = 48-1;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 0xFFFFFFFF;
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
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LED_ACT_Pin|CAN_SILENT_Pin, GPIO_PIN_SET);

  /*Configure GPIO pins : PC13 PC14 PC15 */
  GPIO_InitStruct.Pin = GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PF0 PF1 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /*Configure GPIO pins : PA0 PA1 PA2 PA3 
                           PA4 PA5 PA6 PA7 
                           PA9 PA10 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3 
                          |GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7 
                          |GPIO_PIN_9|GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PB0 PB1 PB2 PB10 
                           PB11 PB12 PB13 PB14 
                           PB15 PB3 PB4 PB5 
                           PB6 PB7 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_10 
                          |GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14 
                          |GPIO_PIN_15|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5 
                          |GPIO_PIN_6|GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : LED_ACT_Pin CAN_SILENT_Pin */
  GPIO_InitStruct.Pin = LED_ACT_Pin|CAN_SILENT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */

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
void assert_failed(char *file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
