#include "indicators.h"

#include "common/timeout.h"
#include "config.h"

#include <stdbool.h>

static TimeoutOnce trig_indicator_timeout = {.inner = {.duration = INPUT_INDICATOR_FLASH_TIME}};
static TimeoutOnce reset_indicator_timeout = {.inner = {.duration = INPUT_INDICATOR_FLASH_TIME}};

/* DECLARATIONS */

/// Calculate the x-coordinate of the indicator LED for the given  output channel
static uint8_t output_channel_led_x(OutputChannel channel);

/* EXTERNAL */

// cppcheck-suppress unusedFunction
void indicators_input_draw(Framebuffer *fb, const InputEvents *events, Milliseconds now) {
	// Flash Trig indicator LED if we received a clock tick
	const bool clock_tick = events->trig || events->internal_clock_tick;
	if (clock_tick) {
		framebuffer_pixel_on(fb, LED_IN_TRIG_X, LED_INDICATORS_Y);
		timeout_once_reset(&trig_indicator_timeout, now);
	}

	// Flash Reset indicator LED if we received a reset input event
	if (events->reset) {
		framebuffer_pixel_on(fb, LED_IN_RESET_X, LED_INDICATORS_Y);
		timeout_once_reset(&reset_indicator_timeout, now);
	}

	// Turn off indicator LEDs that have been on long enough
	if (timeout_once_fired(&trig_indicator_timeout, now)) {
		framebuffer_pixel_off(fb, LED_IN_TRIG_X, LED_INDICATORS_Y);
	}
	if (timeout_once_fired(&reset_indicator_timeout, now)) {
		framebuffer_pixel_off(fb, LED_IN_RESET_X, LED_INDICATORS_Y);
	}
}

// cppcheck-suppress unusedFunction
void indicators_output_latching_draw(Framebuffer *fb, uint8_t out_channels_firing) {
	for (uint8_t out_channel = 0; out_channel < OUTPUT_NUM_CHANNELS; out_channel++) {
		const uint8_t x = output_channel_led_x((OutputChannel)out_channel);

		const uint8_t active_step = (out_channels_firing >> out_channel) & 0x01;

		framebuffer_pixel_set(fb, x, LED_INDICATORS_Y, (Color)active_step);
	}
}

/* INTERNAL */

static uint8_t output_channel_led_x(OutputChannel channel) {
	uint8_t result;
	if (channel == OUTPUT_CHANNEL_1) {
		result = LED_OUT_CH1_X;
	} else if (channel == OUTPUT_CHANNEL_2) {
		result = LED_OUT_CH2_X;
	} else if (channel == OUTPUT_CHANNEL_3) {
		result = LED_OUT_CH3_X;
	} else {
		result = LED_OUT_OFFBEAT_X;
	}
	return result;
}