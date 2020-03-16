#ifndef __CONFIG_H__
#define __CONFIG_H__

#define CAN_QUEUE_SIZE 200

#define SW_VERSION		32  // software version
#define HW_VERSION		13  // hardware version

#define USBD_VID                     0x1d50
#define USBD_PID_FS                  0x606f
#define USBD_LANGID_STRING           1033
#define USBD_CONFIGURATION_STRING_FS (uint8_t*) "cando_usb config"
#define USBD_INTERFACE_STRING_FS     (uint8_t*) "cando_usb interface"

#define USBD_PRODUCT_STRING_FS			(uint8_t*) "Cando"
#define USBD_MANUFACTURER_STRING		(uint8_t*) "ABF"

#endif
