#include "params.h"

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
bool param_name(char *result, Mode mode, ParamIdx idx) {
	if (!result) return false;

	switch (mode) {
		case MODE_EUCLID: {
			if (idx >= EUCLID_NUM_PARAMS) {
				return false;
			}
			memcpy(result, euclid_param_names[idx], PARAM_NAME_LEN);
		} break;
	}

	return true;
}
#endif