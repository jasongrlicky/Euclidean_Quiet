#include "config.h"

#include <Arduino.h>

#include "common/timeout.h"
#include "common/types.h"
#include "hardware/eeprom.h"
#include "hardware/input.h"
#include "hardware/led.h"
#include "hardware/output.h"
#include "hardware/properties.h"
#include "logging.h"
#include "modes/clock.h"
#include "modes/euclid.h"
#include "params.h"
#include "ui/active_channel.h"
#include "ui/framebuffer.h"
#include "ui/framebuffer_led.h"
#include "ui/indicators.h"
#include "ui/led_sleep.h"

#include <euclidean.h>

/* GLOBALS */

static Mode active_mode = MODE_EUCLID;

/* DECLARATIONS */

static void active_mode_switch(Mode mode);
static void mode_init(Mode mode);
static void mode_update(Mode mode, const InputEvents *events, Milliseconds now);
static void mode_params_validate(Params *params, Mode mode);

/* MAIN */

void setup() {
	Milliseconds now = millis();

	logging_init();
	led_init();
	led_sleep_init(now);
	input_init();
	output_init();

	active_mode_switch(MODE_EUCLID);
}

void loop() {
	Milliseconds now = millis();

	log_cycle_time_begin();

	// Input Events
	InputEvents events_in = INPUT_EVENTS_EMPTY;
	input_update(&events_in, now);
	log_input_events(&events_in);

	// Update Internal Clock
	internal_clock_update(&events_in, now);

	// Update Mode
	param_flags_clear_all_modified(&params, active_mode);
	mode_update(active_mode, &events_in, now);
	log_all_modified_params(&params, active_mode);

	// Drawing - Input Indicators
	indicators_input_update(&events_in, now);

	// Update LED Display
	framebuffer_update_color_animations(now);
	framebuffer_copy_row_to_display();

	// Update LED Sleep
	bool postpone_sleep = input_events_contains_any_external(&events_in);
	LedSleepUpdate sleep_update = led_sleep_update(postpone_sleep, now);
	if (sleep_update == LED_SLEEP_UPDATE_WAKE) {
		led_wake();
	} else if (sleep_update == LED_SLEEP_UPDATE_DIM) {
		led_dim();
	} else if (sleep_update == LED_SLEEP_UPDATE_SLEEP) {
		led_sleep();
	}

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

static void mode_init(Mode mode) {
	switch (mode) {
		case MODE_EUCLID:
			euclid_init();
			break;
	}
}

static void mode_update(Mode mode, const InputEvents *events, Milliseconds now) {
	switch (mode) {
		case MODE_EUCLID:
			euclid_update(events, now);
			break;
	}
}

static void mode_params_validate(Params *params, Mode mode) {
	switch (mode) {
		case MODE_EUCLID:
			euclid_params_validate(params);
			break;
	}
}
