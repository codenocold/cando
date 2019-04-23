#ifndef __LED_H__
#define __LED_H__

#include "main.h"
#include <stdint.h>
#include <stdbool.h>

typedef enum {
	led_mode_off,
	led_mode_normal,
	led_mode_warn,
	led_mode_error,
	led_mode_sequence
} led_mode_t;

typedef struct {
	uint8_t state;
	uint8_t time_in_10ms;
} led_seq_step_t;

typedef struct {
	uint32_t on_until;
	uint32_t off_until;
} led_state_t;

typedef struct {
	led_mode_t mode;
	led_mode_t last_mode;

	led_seq_step_t *sequence;
	uint32_t sequence_step;
	uint32_t t_sequence_next;
	int32_t seq_num_repeat;

	led_state_t led_state;
} led_data_t;

#define LED_SET()	LED_ACT_GPIO_Port->BRR  = (uint32_t)LED_ACT_Pin
#define LED_CLR()	LED_ACT_GPIO_Port->BSRR = (uint32_t)LED_ACT_Pin
#define LED_TOG()	LED_ACT_GPIO_Port->ODR  ^= LED_ACT_Pin;

void led_init(led_data_t *leds);
void led_set_mode(led_data_t *leds,led_mode_t mode);
void led_run_sequence(led_data_t *leds, led_seq_step_t *sequence, int32_t num_repeat);
void led_indicate_trx(led_data_t *leds);
void led_update(led_data_t *leds);

#endif
