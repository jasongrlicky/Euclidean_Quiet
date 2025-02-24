#include "framebuffer_led.h"

#include "common/timeout.h"
#include "config.h"
#include "framebuffer.h"
#include "hardware/led.h"

extern Framebuffer framebuffer;

/// To keep latency from spiking, we only draw one row of the framebuffer to the
/// LED matrix at a time. The row that gets drawn rotates between the 8 rows of
/// the framebuffer to keep visual latency equal for all rows.
static uint8_t out_row;

#define ANIM_BLINK_NUM_FRAMES 2
static Timeout anim_blink_timeout = {.duration = ANIM_BLINK_INTERVAL};
static uint8_t anim_blink_frame = 0;

#define ANIM_ANTS_NUM_FRAMES 4
static Timeout anim_ants_timeout = {.duration = ANIM_ANTS_INTERVAL};
static uint8_t anim_ants_frame = 0;

/* DECLARATIONS */

static inline uint8_t anim_marching_ants(uint8_t frame, uint8_t x, uint8_t y);

/* EXTERNAL */

// cppcheck-suppress unusedFunction
void framebuffer_copy_row_to_display() {
	const uint8_t row = (out_row) % LED_ROWS;
	const uint16_t row_bits = framebuffer.data[row];

	uint8_t to_draw = 0;
	for (uint8_t col = 0; col < LED_COLUMNS; col++) {
		Color color = (Color)((row_bits >> (col * 2)) & 0b00000011);

		if (color == COLOR_ANTS) {
			to_draw |= (anim_marching_ants(anim_ants_frame, col, row) << col);
		} else if (color == COLOR_BLINK) {
			to_draw |= (anim_blink_frame << col);
		} else {
			to_draw |= (color << col);
		}
	}

	led_set_row(row, to_draw);

	// Next cycle, copy the next row of the framebuffer to the LED matrix
	out_row = (out_row + 1) % LED_ROWS;
}

// cppcheck-suppress unusedFunction
void framebuffer_update_color_animations(Milliseconds now) {
	if (timeout_loop(&anim_blink_timeout, now)) {
		anim_blink_frame = (anim_blink_frame + 1) % ANIM_BLINK_NUM_FRAMES;
	}

	if (timeout_loop(&anim_ants_timeout, now)) {
		anim_ants_frame = (anim_ants_frame + 1) % ANIM_ANTS_NUM_FRAMES;
	}
}

/* INTERNAL */

static inline uint8_t anim_marching_ants(uint8_t frame, uint8_t x, uint8_t y) {
	const uint8_t val = (x + y + (ANIM_ANTS_NUM_FRAMES - frame)) / 2;
	return (val % 2);
}