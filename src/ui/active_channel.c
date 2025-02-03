#include "active_channel.h"

#include "common/framebuffer.h"
#include "hardware/properties.h"

#include <stdint.h>

void active_channel_display_draw(Channel active_channel) {
	uint16_t row_bits = 0;
	if (active_channel == CHANNEL_1) {
		row_bits = 0x0005; // Two left dots
	} else if (active_channel == CHANNEL_2) {
		row_bits = 0x0140; // Two middle dots
	} else if (active_channel == CHANNEL_3) {
		row_bits = 0x5000; // Two right dots
	}
	framebuffer_row_set(LED_CH_SEL_Y, row_bits);
}
