#include "params.h"

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
