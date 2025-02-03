#include "indicators.h"

#include "framebuffer.h"

/* DECLARATIONS */

/// Calculate the x-coordinate of the indicator LED for the given  output channel
static uint8_t output_channel_led_x(OutputChannel channel);

/* EXTERNAL */

void draw_output_indicators(uint8_t out_channels_firing) {
	for (uint8_t out_channel = 0; out_channel < OUTPUT_NUM_CHANNELS; out_channel++) {
		uint8_t x = output_channel_led_x((OutputChannel)out_channel);

		uint8_t active_step = (out_channels_firing >> out_channel) & 0x01;

		framebuffer_pixel_set(x, LED_INDICATORS_Y, (Color)active_step);
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