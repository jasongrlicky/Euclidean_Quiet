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

static Milliseconds last_clock_or_reset;

static TimeoutOnce output_pulse_timeout = {
    .inner = {.duration = 5}}; // Pulse length, set based on the time since last trigger

// Track the time since the playhead has moved so we can make it flash in its idle loop
static Timeout playhead_idle_timeout = {.duration = PLAYHEAD_IDLE_TIME};
// Loop for making the playhead flash periodically after it is idle
static Timeout playhead_idle_loop_timeout = {.duration = PLAYHEAD_IDLE_LOOP_PERIOD};

static Timeout adjustment_display_timeout = {.duration = ADJUSTMENT_DISPLAY_TIME};

static Mode active_mode = MODE_EUCLID;

/* DECLARATIONS */

static Milliseconds calc_playhead_flash_time(Milliseconds clock_period);
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

	led_wake();

	// Select first channel on startup
	euclid_state.active_channel = CHANNEL_1;

	// Draw initial UI
	euclid_draw_channels();
	active_channel_display_draw(euclid_state.active_channel);
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

	euclid_handle_encoder_push(events_in.enc_push);

	// Note the param associated with a knob that was moved so we can re-generate
	// the Euclidean rhythms and show the adjustment display.
	EuclidParamOpt param_knob_moved = euclid_handle_encoder_move(events_in.enc_move);

	// Update Generated Rhythms Based On Parameter Changes
	Channel active_channel = euclid_state.active_channel;
	if (param_knob_moved.valid) {
		Channel channel = active_channel;
		uint8_t length = euclid_get_length(&params, channel);
		uint8_t density = euclid_get_density(&params, channel);
		uint8_t offset = euclid_get_offset(&params, channel);

		generated_rhythms[channel] = euclidean_pattern_rotate(length, density, offset);
	}

	/* UPDATE SEQUENCER */

	// Clock ticks merge the internal and external clocks
	bool clock_tick = events_in.trig || events_in.internal_clock_tick;

	// Tracks if any of the sequencers' states have been updated this cycle
	bool sequencers_updated = (clock_tick || events_in.reset);

	// Bitflags storing which output channels will fire this cycle, indexed by
	// `OutputChannel`.
	uint8_t out_channels_firing = euclid_update(&events_in);

	/* OUTPUT */

	for (uint8_t out_channel = 0; out_channel < OUTPUT_NUM_CHANNELS; out_channel++) {
		bool should_fire = out_channels_firing & (0x01 << out_channel);
		if (should_fire) {
			output_set_high((OutputChannel)out_channel);
		}
	}

	if (sequencers_updated) {
		// Update output pulse length and timeout
		Milliseconds pulse_length = constrain(((now - output_pulse_timeout.inner.start) / 5), 2, 5);
		output_pulse_timeout.inner.duration = pulse_length;

		timeout_once_reset(&output_pulse_timeout, now);
	}

	// FINISH ANY PULSES THAT ARE ACTIVE
	if (timeout_once_fired(&output_pulse_timeout, now)) {
		output_clear_all();
	}

	/* DRAWING - ACTIVE CHANNEL DISPLAY */

	if (events_in.enc_push != ENCODER_NONE) {
		active_channel_display_draw(active_channel);
	}

	/* DRAWING - CHANNELS */

	if (sequencers_updated) {
		// Update playhead flash duration based on the last interval between two
		// clock or reset signals received.
		Milliseconds previous_period = now - last_clock_or_reset;
		playhead_flash_timeout.inner.duration = calc_playhead_flash_time(previous_period);
		last_clock_or_reset = now;

		// Reset playhead flash
		timeout_once_reset(&playhead_flash_timeout, now);

		// Reset playhead idle
		timeout_reset(&playhead_idle_timeout, now);
	}

	// Update playhead idle - Make playhead flash periodically when it hasn't
	// moved in a certain amount of time
	bool playhead_flash_updated = false;
	if (timeout_fired(&playhead_idle_timeout, now)) {
		if (timeout_loop(&playhead_idle_loop_timeout, now)) {
			playhead_flash_timeout.inner.duration = PLAYHEAD_FLASH_TIME_DEFAULT;
			timeout_once_reset(&playhead_flash_timeout, now);
			playhead_flash_updated = true;
		}
	}

	// Update playhead flash
	if (timeout_once_fired(&playhead_flash_timeout, now)) {
		playhead_flash_updated = true;
	}

	// Tracks if the screen needs to be redrawn.
	bool needs_redraw = sequencers_updated || playhead_flash_updated;

	if (param_knob_moved.valid) {
		// If parameters have changed, reset the adjustment display timeout and state
		adjustment_display_state.channel = active_channel;
		adjustment_display_state.parameter = param_knob_moved.inner;
		adjustment_display_state.visible = true;
		timeout_reset(&adjustment_display_timeout, now);

		needs_redraw = true;
	} else {
		// If no parameters have changed, check if the adjustment display still
		// needs to be shown, and hide it if it doesn't
		if (adjustment_display_state.visible) {
			bool should_be_hidden = timeout_fired(&adjustment_display_timeout, now);
			if (should_be_hidden) {
				adjustment_display_state.visible = false;
				needs_redraw = true;
			}
		}
	}

	if (needs_redraw) {
		euclid_draw_channels();
	}

	/* DRAWING - INDICATORS */

	indicators_input_update(&events_in, now);

	if (sequencers_updated) {
		indicators_output_draw_latching(out_channels_firing);
	}

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

static Milliseconds calc_playhead_flash_time(Milliseconds clock_period) {
	// This is a standard "scale from input range to output range" function, but
	// it uses specific ranges so that we can avoid multiplication or division by
	// numbers that aren't powers of 2.

	// 256ms min period = ~234bpm
	// 1280ms max period = ~47bpm
	// 1280-256 = an input range of 1024, or 2^10
	clock_period = constrain(clock_period, 256, 1280);
	// Subtract input min
	Milliseconds delta = clock_period - 256;
	// (delta / input range) * output range. Input range is 2^10, output range is
	// 2^7, so just divide by 2^3.
	Milliseconds result = delta >> 3;
	// Add output min
	result += 64;
	return result;
}

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