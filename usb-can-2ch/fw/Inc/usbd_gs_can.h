#ifndef __USBD_GS_CAN_H__
#define __USBD_GS_CAN_H__

#include <stdbool.h>
#include "usbd_def.h"
#include "queue.h"
#include "led.h"
#include "can.h"
#include "gs_usb.h"

/* Define these here so they can be referenced in other files */
#define CAN_DATA_MAX_PACKET_SIZE   32  /* Endpoint IN & OUT Packet size */
#define CAN_CMD_PACKET_SIZE        64  /* Control Endpoint Packet size */
#define USB_CAN_CONFIG_DESC_SIZ    32
#define NUM_CAN_CHANNEL             2
#define USBD_GS_CAN_VENDOR_CODE  0x20

extern USBD_ClassTypeDef USBD_GS_CAN;

uint8_t USBD_GS_CAN_Init(USBD_HandleTypeDef *pdev, queue_t *q_frame_pool, queue_t *q_from_host);
void USBD_GS_CAN_SetChannel(USBD_HandleTypeDef *pdev, uint8_t channel, can_data_t* handle);
bool USBD_GS_CAN_TxReady(USBD_HandleTypeDef *pdev);
uint8_t USBD_GS_CAN_PrepareReceive(USBD_HandleTypeDef *pdev);
bool USBD_GS_CAN_CustomDeviceRequest(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
bool USBD_GS_CAN_CustomInterfaceRequest(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);

uint8_t USBD_GS_CAN_SendFrame(USBD_HandleTypeDef *pdev, struct gs_host_frame *frame);
uint8_t USBD_GS_CAN_Transmit(USBD_HandleTypeDef *pdev, uint8_t *buf, uint16_t len);
uint8_t USBD_GS_CAN_GetProtocolVersion(USBD_HandleTypeDef *pdev);
uint8_t USBD_GS_CAN_GetPadPacketsToMaxPacketSize(USBD_HandleTypeDef *pdev);

#endif
