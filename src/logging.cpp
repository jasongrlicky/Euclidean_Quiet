#include "logging.h"

#include "common/timeout.h"

#include <Arduino.h>

/* GLOBALS */

#if LOGGING_ENABLED && LOGGING_CYCLE_TIME
static Microseconds cycle_time_start;
static Microseconds cycle_time_max;
static Timeout log_cycle_time_timeout = {.duration = LOGGING_CYCLE_TIME_INTERVAL};
#endif

/* EXTERNAL */

void logging_init() {
#if LOGGING_ENABLED
	Serial.begin(9600);
#endif
}

void log_cycle_time_begin() {
#if LOGGING_ENABLED && LOGGING_CYCLE_TIME
	cycle_time_start = micros();
#endif
}

void log_cycle_time_end(Milliseconds now) {
#if LOGGING_ENABLED && LOGGING_CYCLE_TIME
	const Microseconds cycle_time = micros() - cycle_time_start;
	if (cycle_time > cycle_time_max) {
		cycle_time_max = cycle_time;
	}

	if (timeout_loop(&log_cycle_time_timeout, now)) {
		Serial.print("Max Cycle Time: ");
		Serial.println(cycle_time_max);
		cycle_time_max = 0;
	}
#endif
}

void log_eeprom_write(Mode mode, ParamIdx idx, Address addr, uint8_t val) {
#if LOGGING_ENABLED && LOGGING_EEPROM
	char name[PARAM_NAME_LEN];
	param_name(name, mode, idx);

	Serial.print("EEPROM Write: ");
	Serial.print(name);
	Serial.print(" @");
	Serial.print(addr);
	Serial.print(": ");
	Serial.println(val);
#endif
}

void log_input_events(const InputEvents *events) {
#if LOGGING_ENABLED && LOGGING_INPUT
	if (events->reset) {
		Serial.println("INPUT: Reset");
	}
	if (events->trig) {
		Serial.println("INPUT: Trigger");
	}
	if (events->enc_move[ENCODER_1] != 0) {
		Serial.print("ENC_1: Move ");
		Serial.println(events->enc_move[ENCODER_1]);
	}
	if (events->enc_move[ENCODER_2] != 0) {
		Serial.print("ENC_2: Move ");
		Serial.println(events->enc_move[ENCODER_2]);
	}
	if (events->enc_move[ENCODER_3] != 0) {
		Serial.print("ENC_3: Move ");
		Serial.println(events->enc_move[ENCODER_3]);
	}
#endif
}

void log_all_modified_params(const Params *params, Mode mode) {
#if LOGGING_ENABLED
	const uint8_t num_params = mode_num_params[mode];

	for (uint8_t idx = 0; idx < num_params; idx++) {
		bool modified = param_flags_get(params, idx, PARAM_FLAG_MODIFIED);
		if (!modified) continue;

		uint8_t val = params->values[idx];
		char name[PARAM_NAME_LEN];
		param_name(name, mode, idx);

		Serial.print("Param ");
		Serial.print(name);
		Serial.print(": ");
		Serial.println(val);
	}
#endif
}