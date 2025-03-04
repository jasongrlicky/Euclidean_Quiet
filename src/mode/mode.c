#include "mode.h"

#include "mode/euclid.h"

#if LOGGING_ENABLED
#include <string.h>
#endif

/* CONSTANTS */

/*
Original EEPROM Schema:
Channel 1: length = 1 density = 2 offset = 7
Channel 2: length = 3 density = 4 offset = 8
Channel 3: length = 5 density = 6 offset = 9
*/
/// EEPROM addresses for Euclidean mode params. The order is this way for
/// backwards-compatibility with the original Sebsongs Euclidean firmware.
static const Address euclid_param_addresses[EUCLID_NUM_PARAMS] = {1, 2, 7, 3, 4, 8, 5, 6, 9};

#if LOGGING_ENABLED
/// Table of parameter names for logging in the Euclid mode
static const char euclid_param_names[EUCLID_NUM_PARAMS][PARAM_NAME_LEN] = {
    "L1", "D1", "O1", "L2", "D2", "O2", "L3", "D3", "O3",
};

/// Value that is returned if the param name is not found
static const char name_not_found[PARAM_NAME_LEN] = "??";
#endif

/* EXTERNAL */

void mode_init(ModeState *state, Params *params, Framebuffer *fb, Mode mode) {
	switch (mode) {
		case MODE_EUCLID:
			euclid_init(&state->euclid, params, fb);
			break;
	}
}

void mode_update(ModeState *state, Params *params, Framebuffer *fb, Mode mode, const InputEvents *events,
                 Milliseconds now) {
	switch (mode) {
		case MODE_EUCLID:
			euclid_update(&state->euclid, params, fb, events, now);
			break;
	}
}

void mode_params_validate(Params *params, Mode mode) {
	switch (mode) {
		case MODE_EUCLID:
			euclid_params_validate(params);
			break;
	}
}

Address mode_param_address(Mode mode, ParamIdx idx) {
	Address result = 0;
	switch (mode) {
		case MODE_EUCLID: {
			if (idx < EUCLID_NUM_PARAMS) {
				result = euclid_param_addresses[idx];
			}
		} break;
	}
	return result;
}

#if LOGGING_ENABLED
void mode_param_name(char *result, Mode mode, ParamIdx idx) {
	// Early return - null pointer
	if (!result) return;

	switch (mode) {
		case MODE_EUCLID: {
			const char *name_source = (idx < EUCLID_NUM_PARAMS) ? euclid_param_names[idx] : name_not_found;
			memcpy(result, name_source, PARAM_NAME_LEN);
		} break;
	}
}
#endif