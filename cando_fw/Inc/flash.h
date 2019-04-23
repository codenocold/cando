#ifndef __FLASH_H__
#define __FLASH_H__

#include <stdbool.h>
#include <stdint.h>

void flash_load(void);
bool flash_set_user_id(uint8_t channel, uint32_t user_id);
uint32_t flash_get_user_id(uint8_t channel);
void flash_flush(void);

#endif
