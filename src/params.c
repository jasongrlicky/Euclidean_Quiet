#include "params.h"
/* CONSTANTS */

#if LOGGING_ENABLED
static const char name_not_found[PARAM_NAME_LEN] = "??";
#endif

/* GLOBALS */

Params params;

/* INTERNAL */

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
#endif

/* EXTERNAL */

void param_and_flags_set(Params *params, ParamIdx idx, uint8_t value) {
	params->values[idx] = value;
	param_flags_set(params, idx, (PARAM_FLAG_MODIFIED | PARAM_FLAG_NEEDS_WRITE));
}

uint8_t param_flags_get(const Params *params, ParamIdx idx, uint8_t mask) {
	return (params->flags[idx] & mask);
}

void param_flags_set(Params *params, ParamIdx idx, uint8_t mask) { params->flags[idx] |= mask; }

void param_flags_clear(Params *params, ParamIdx idx, uint8_t mask) { params->flags[idx] &= ~mask; }

void param_flags_clear_all_modified(Params *params, Mode mode) {
	const uint8_t num_params = mode_num_params[mode];

	for (uint8_t idx = 0; idx < num_params; idx++) {
		param_flags_clear(params, idx, PARAM_FLAG_MODIFIED);
	}
}

Address param_address(Mode mode, ParamIdx idx) {
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
void param_name(char *result, Mode mode, ParamIdx idx) {
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