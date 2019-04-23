#include "dfu.h"
#include <stdint.h>
#include "stm32f0xx_hal.h"

#define RESET_TO_BOOTLOADER_MAGIC_CODE 0xDEADBEEF
#define SYSMEM_STM32F072 0x1FFFC800

static uint32_t dfu_reset_to_bootloader_magic;

static void dfu_jump_to_bootloader(uint32_t sysmem_base);

void dfu_run_bootloader()
{
	dfu_reset_to_bootloader_magic = RESET_TO_BOOTLOADER_MAGIC_CODE;
	NVIC_SystemReset();
}

void __initialize_hardware_early(void)
{
	if (dfu_reset_to_bootloader_magic == RESET_TO_BOOTLOADER_MAGIC_CODE){
		dfu_jump_to_bootloader(SYSMEM_STM32F072);
	}

	SystemInit();
}

static void dfu_jump_to_bootloader(uint32_t sysmem_base)
{
	void (*bootloader)(void) = (void (*)(void)) (*((uint32_t *) (sysmem_base + 4)));

	__set_MSP(*(__IO uint32_t*) sysmem_base);
	bootloader();

	while (42){

	}
}
