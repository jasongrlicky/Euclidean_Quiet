#include "mode_params.h"

#include "modes/euclid.h"

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

/* EXTERNAL */

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
