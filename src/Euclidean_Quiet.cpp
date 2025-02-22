#include "config.h"

#include <Arduino.h>

#include "common/types.h"
#include "hardware/eeprom.h"
#include "hardware/input.h"
#include "hardware/led.h"
#include "hardware/output.h"
#include "hardware/properties.h"
#include "logging.h"
#include "mode/clock.h"
#include "mode/euclid.h"
#include "mode/mode.h"
#include "params.h"
#include "ui/framebuffer.h"
#include "ui/framebuffer_led.h"
#include "ui/indicators.h"
#include "ui/led_sleep.h"

/* GLOBALS */

static Mode active_mode = MODE_EUCLID;

/* DECLARATIONS */

static void active_mode_switch(Mode mode);
/// Clear `PARAM_FLAG_MODIFIED` for all parameters
static void params_reset_modified_flag(Params *params, Mode mode);

/* MAIN */

void setup() {
	const Milliseconds now = millis();

	logging_init();
	led_init();
	led_sleep_init(now);
	input_init();
	output_init();

	active_mode_switch(MODE_EUCLID);
}

void loop() {
	const Milliseconds now = millis();

	log_cycle_time_begin();

	// Input Events
	InputEvents events_in = INPUT_EVENTS_EMPTY;
	input_update(&events_in, now);
	log_input_events(&events_in);

	// Update Internal Clock
	internal_clock_update(&events_in, now);

	// Update Active Mode
	params_reset_modified_flag(&params, active_mode);
	mode_update(active_mode, &events_in, now);
	log_all_modified_params(&params, active_mode);

	// Drawing - Input Indicators
	indicators_input_update(&events_in, now);

	// Update LED Display
	framebuffer_update_color_animations(now);
	framebuffer_copy_row_to_display();

	// Update LED Sleep
	const bool postpone_sleep = input_events_contains_any_external(&events_in);
	led_sleep_update(postpone_sleep, now);

	// EEPROM Writes
	eeprom_save_all_needing_write(&params, active_mode);

	log_cycle_time_end(now);
}

/* INTERNAL */

static void active_mode_switch(Mode mode) {
	active_mode = mode;

	eeprom_params_load(&params, mode);
	mode_params_validate(&params, mode);

	mode_init(mode);
}

void params_reset_modified_flag(Params *params, Mode mode) {
	const uint8_t num_params = mode_num_params[mode];

	for (uint8_t idx = 0; idx < num_params; idx++) {
		param_flags_clear(params, idx, PARAM_FLAG_MODIFIED);
	}
}
