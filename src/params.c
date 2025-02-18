#include "params.h"

static const char name_not_found[PARAM_NAME_LEN] = "??";

Address param_address(Mode mode, ParamIdx idx) {
	Address result = 0;
	switch (mode) {
		case MODE_EUCLID:
			result = euclid_param_addresses[idx];
			break;
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