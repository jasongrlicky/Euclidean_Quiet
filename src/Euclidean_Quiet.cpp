#include "config.h"

#if EEPROM_READ || EEPROM_WRITE
#include <EEPROM.h>
#endif

#include <Arduino.h>

#include "common/timeout.h"
#include "common/types.h"
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
static void params_validate(Params *params, Mode mode);
/// Load state for the given mode into `params`.
static void eeprom_params_load(Params *params, Mode mode);
static void eeprom_save_all_needing_write(Params *params, Mode mode);

/* MAIN */

void setup() {
	Milliseconds now = millis();

	logging_init();
	led_init();
	led_sleep_init(now);
	active_mode_switch(MODE_EUCLID);
	input_init();
	output_init();

	// Initialise generated rhythms
	for (int a = 0; a < NUM_CHANNELS; a++) {
		Channel channel = (Channel)a;
		uint8_t length = euclid_get_length(&params, channel);
		uint8_t density = euclid_get_density(&params, channel);
		uint8_t offset = euclid_get_offset(&params, channel);
		generated_rhythms[a] = euclidean_pattern_rotate(length, density, offset);
	}

	// Select first channel on startup
	euclid_state.active_channel = CHANNEL_1;

	// Draw initial UI
	euclid_draw_channels();
	active_channel_display_draw(euclid_state.active_channel);

	led_wake();
}

void loop() {
	Milliseconds now = millis();

	log_cycle_time_begin();

	/* INPUT EVENTS */

	InputEvents events_in = INPUT_EVENTS_EMPTY;
	input_update(&events_in, now);
	log_input_events(&events_in);

	/* UPDATE INTERNAL CLOCK */

	internal_clock_update(&events_in, now);

	/* HANDLE INPUT */

	param_flags_clear_all_modified(&params, active_mode);

	/* UPDATE MODE */

	euclid_update(&events_in, now);

	/* DRAWING - INPUT INDICATORS */

	indicators_input_update(&events_in, now);

	/* UPDATE LED DISPLAY */

	framebuffer_update_color_animations(now);
	framebuffer_copy_row_to_display();

	/* UPDATE LED SLEEP */

	bool postpone_sleep = input_events_contains_any_external(&events_in);
	LedSleepUpdate sleep_update = led_sleep_update(postpone_sleep, now);
	if (sleep_update == LED_SLEEP_UPDATE_WAKE) {
		led_wake();
	} else if (sleep_update == LED_SLEEP_UPDATE_DIM) {
		led_dim();
	} else if (sleep_update == LED_SLEEP_UPDATE_SLEEP) {
		led_sleep();
	}

	/* EEPROM WRITES */

	eeprom_save_all_needing_write(&params, active_mode);

	log_cycle_time_end(now);

	log_all_modified_params(&params, active_mode);
}

/* INTERNAL */

static void active_mode_switch(Mode mode) {
	active_mode = mode;
	eeprom_params_load(&params, mode);
	params_validate(&params, mode);
}

static void params_validate(Params *params, Mode mode) {
	switch (mode) {
		case MODE_EUCLID:
			euclid_params_validate(params);
			break;
	}
}

static void eeprom_params_load(Params *params, Mode mode) {
	uint8_t num_params = mode_num_params[mode];

	for (uint8_t idx = 0; idx < num_params; idx++) {
#if EEPROM_READ
		Address addr = param_address(mode, (ParamIdx)idx);
		params->values[idx] = EEPROM.read(addr);
#else
		params.values[idx] = 0;
#endif
		params->flags[idx] = PARAM_FLAGS_NONE;
	}

	params->len = num_params;
}

static void eeprom_save_all_needing_write(Params *params, Mode mode) {
#if EEPROM_WRITE
	uint8_t num_params = mode_num_params[mode];

	for (uint8_t idx = 0; idx < num_params; idx++) {
		bool needs_write = param_flags_get(params, idx, PARAM_FLAG_NEEDS_WRITE);
		if (!needs_write) continue;

		param_flags_clear(params, idx, PARAM_FLAG_NEEDS_WRITE);

		uint8_t val = params->values[idx];
		Address addr = param_address(mode, (ParamIdx)idx);
		EEPROM.write(addr, val);

		log_eeprom_write(mode, idx, addr, val);
	}
#endif
}