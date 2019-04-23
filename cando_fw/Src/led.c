#include "led.h"
#include <string.h>
#include "stm32f0xx_hal.h"

extern TIM_HandleTypeDef htim1;

void led_init(led_data_t *leds)
{
	memset(leds, 0, sizeof(led_data_t));
}

void led_set_mode(led_data_t *leds, led_mode_t mode)
{
	leds->mode = mode;
	led_update(leds);
}

static uint32_t led_set_sequence_step(led_data_t *leds, uint32_t step_num)
{
	led_seq_step_t *step = &leds->sequence[step_num];
	leds->sequence_step = step_num;
	if(step->state & 0x01){
		LED_SET();
	}else{
		LED_CLR();
	}
	leds->t_sequence_next = HAL_GetTick() + 10*step->time_in_10ms;
	return 10 * step->time_in_10ms;
}

void led_run_sequence(led_data_t *leds, led_seq_step_t *sequence, int32_t num_repeat)
{
	leds->last_mode = leds->mode;
	leds->mode = led_mode_sequence;
	leds->sequence = sequence;
	leds->seq_num_repeat = num_repeat;
	led_set_sequence_step(leds, 0);
	led_update(leds);
}

void led_indicate_trx(led_data_t *leds)
{
	uint32_t now = HAL_GetTick();
	led_state_t *led = &leds->led_state;

	if ( (led->on_until < now) && (led->off_until < now) ) {
		led->off_until = now + 30;
		led->on_until = now + 45;
	}

	led_update(leds);
}

static void led_update_normal_mode(led_state_t *led)
{
	uint32_t now = HAL_GetTick();
	if(led->off_until < now){
		LED_SET();
	}else{
		LED_CLR();
	}
}

static void led_update_sequence(led_data_t *leds)
{
	if (leds->sequence == NULL) {
		return;
	}

	uint32_t now = HAL_GetTick();
	if (now > leds->t_sequence_next) {
		uint32_t t = led_set_sequence_step(leds, ++leds->sequence_step);
		if (t > 0) { // the saga continues
			leds->t_sequence_next = now + t;
		} else { // end of sequence
			if (leds->seq_num_repeat != 0) {
				if (leds->seq_num_repeat > 0) {
					leds->seq_num_repeat--;
				}
				led_set_sequence_step(leds, 0);
			} else {
				leds->sequence = NULL;
			}
		}
	}
}

void led_update(led_data_t *leds)
{
	switch (leds->mode) {
		case led_mode_off:
			LED_CLR();
			break;

		case led_mode_normal:
			led_update_normal_mode(&leds->led_state);
			break;

		case led_mode_sequence:
			led_update_sequence(leds);
			break;

		default:
			LED_CLR();
			break;
	}
}
