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

bool param_name(char *result, Mode mode, ParamIdx idx) {
	if (!result) return false;

	memcpy(result, param_names[idx], PARAM_NAME_LEN);
	return true;
}