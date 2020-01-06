#ifndef __CAN_H__
#define __CAN_H__

#include <stdint.h>
#include <stdbool.h>
#include "stm32f0xx_hal.h"
#include "gs_usb.h"

typedef struct {
	CAN_TypeDef *instance;
	uint16_t brp;
	uint8_t phase_seg1;
	uint8_t phase_seg2;
	uint8_t sjw;
} can_data_t;

void can_init(can_data_t *hcan, CAN_TypeDef *instance);
bool can_set_bittiming(can_data_t *hcan, uint16_t brp, uint8_t phase_seg1, uint8_t phase_seg2, uint8_t sjw);
void can_enable(can_data_t *hcan, bool loop_back, bool listen_only, bool one_shot);
void can_disable(can_data_t *hcan);
bool can_is_enabled(can_data_t *hcan);

bool can_receive(can_data_t *hcan, struct gs_host_frame *rx_frame);
bool can_is_rx_pending(can_data_t *hcan);

bool can_send(can_data_t *hcan, struct gs_host_frame *frame);

uint32_t can_get_error_status(can_data_t *hcan);
bool can_parse_error_status(can_data_t *hcan, uint32_t err, struct gs_host_frame *frame);

#endif
