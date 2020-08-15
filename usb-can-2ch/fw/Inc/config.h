#ifndef __CONFIG_H__
#define __CONFIG_H__

#define CAN_QUEUE_SIZE  400

#define SW_VERSION		33  // software version
#define HW_VERSION		15  // hardware version

#define USBD_VID                     	0x1d50
#define USBD_PID_FS                  	0x606f
#define USBD_LANGID_STRING           	0x0409		// Language id China: 0x0804; US: 0x0409
#define USBD_PRODUCT_STRING_FS			(uint8_t*) "Cando_2ch"
#define USBD_MANUFACTURER_STRING		(uint8_t*) "ABF"
#define USBD_CONFIGURATION_STRING_FS 	(uint8_t*) "cando_usb config"
#define USBD_INTERFACE_STRING_FS     	(uint8_t*) "cando_usb interface"

#endif
