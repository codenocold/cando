#ifndef __USBD_CONF_H__
#define __USBD_CONF_H__

#include "stm32f0xx_hal.h"

#define USBD_MAX_NUM_INTERFACES      1
#define USBD_MAX_NUM_CONFIGURATION   1
#define USBD_MAX_STR_DESC_SIZ      512
#define USBD_SUPPORT_USER_STRING     1
#define USBD_SELF_POWERED            0
#define DEVICE_FS                    0

#define USBD_ErrLog(...)

#endif
