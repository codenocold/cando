#pragma once

#include "cando_defs.h"

enum {
    CANDO_DEVMODE_RESET = 0,
    CANDO_DEVMODE_START = 1
};

bool cando_ctrl_set_bittiming(cando_device_t *dev, uint16_t ch, cando_bittiming_t *timing);
bool cando_ctrl_set_device_mode(cando_device_t *dev, uint16_t ch, uint32_t mode, uint32_t flags);
bool cando_ctrl_get_config(cando_device_t *dev, cando_device_config_t *dconf);
