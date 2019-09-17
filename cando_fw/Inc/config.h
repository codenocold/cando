#ifndef __CONFIG_H__
#define __CONFIG_H__

#define CAN_QUEUE_SIZE 200

#define USBD_VID                     0x1d50
#define USBD_PID_FS                  0x606f
#define USBD_LANGID_STRING           1033
#define USBD_CONFIGURATION_STRING_FS (uint8_t*) "gs_usb config"
#define USBD_INTERFACE_STRING_FS     (uint8_t*) "gs_usb interface"

#define USBD_PRODUCT_STRING_FS			(uint8_t*) "Cando"
#define USBD_MANUFACTURER_STRING		(uint8_t*) "Cando.abf"
#define DFU_INTERFACE_STRING_FS			(uint8_t*) "Cando firmware upgrade interface"

#endif
