#include "flash.h"
#include "stm32f0xx_hal.h"
#include <string.h>
#include "usbd_gs_can.h"

#define NUM_CHANNEL NUM_CAN_CHANNEL

typedef struct {
	uint32_t user_id[NUM_CHANNEL];
} flash_data_t;

static flash_data_t flash_data_ram;
static const flash_data_t flash_data_rom __attribute__((at(0x0801F800)));	// Page 63, Page size 2Kbytes

void flash_load(void)
{
	memcpy(&flash_data_ram, &flash_data_rom, sizeof(flash_data_t));
}

bool flash_set_user_id(uint8_t channel, uint32_t user_id)
{
	if (channel<NUM_CHANNEL) {
		if (flash_data_ram.user_id[channel] != user_id) {
			flash_data_ram.user_id[channel] = user_id;
			flash_flush();
		}
		return true;
	} else {
		return false;
	}
}

uint32_t flash_get_user_id(uint8_t channel)
{
	if (channel<NUM_CHANNEL) {
		return flash_data_ram.user_id[channel];
	} else {
		return 0;
	}
}

void flash_flush(void)
{
	FLASH_EraseInitTypeDef erase_pages;
	erase_pages.PageAddress = (uint32_t)&flash_data_rom;
	erase_pages.NbPages = 1;
	erase_pages.TypeErase = FLASH_TYPEERASE_PAGES;

	uint32_t error;

	HAL_FLASH_Unlock();
	__HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_WRPERR | FLASH_SR_PGERR);
	HAL_FLASHEx_Erase(&erase_pages, &error);
	if (error==0xFFFFFFFF) { // erase finished successfully
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, (uint32_t)&flash_data_rom.user_id[0], flash_data_ram.user_id[0]);
	}
	HAL_FLASH_Lock();
}
