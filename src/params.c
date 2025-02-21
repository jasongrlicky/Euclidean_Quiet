#include "params.h"

#include "modes/euclid.h"

/* CONSTANTS */

#if LOGGING_ENABLED
static const char name_not_found[PARAM_NAME_LEN] = "??";
#endif

/* GLOBALS */

Params params;

/* INTERNAL */

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
