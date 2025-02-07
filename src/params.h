#ifndef PARAMS_H_
#define PARAMS_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "common/types.h"
#include "config.h"

#include <stdint.h>

#define NUM_MODES 1
typedef enum Mode {
	MODE_EUCLID,
} Mode;

#define EUCLID_NUM_PARAMS 9

/// How many params this mode has. Indexed by the `Mode` enum.
static const uint8_t mode_param_num[NUM_MODES] = {
    EUCLID_NUM_PARAMS, // EUCLID
};

/// EEPROM addresses for Euclidean mode params. The order is this way for
/// backwards-compatibility with the original Sebsongs Euclidean firmware.
static const Address euclid_param_addresses[EUCLID_NUM_PARAMS] = {1, 2, 7, 3, 4, 8, 5, 6, 9};

Address param_address(Mode mode, ParamIdx idx) {
	Address result = 0;
	switch (mode) {
		case MODE_EUCLID:
			result = euclid_param_addresses[idx];
			break;
	}
	return result;
}

// clang-format off
#if LOGGING_ENABLED

#define PARAM_NAME_LEN 3
/// Table of parameter names for logging
static const char param_names[EUCLID_NUM_PARAMS][PARAM_NAME_LEN] = {
	// Euclid Mode
	"L1", // Length, Channel 1
	"D1", // Density, Channel 1
	"O1", // Offset, Channel 1
	"L2",
	"D2",
	"O2",
	"L3",
	"D3",
	"O3",
};

// TODO: This implementation is brittle and will not scale with more modes. 
bool param_name(char *result, Mode mode, ParamIdx idx) {
	if (!result) return false;

	memcpy(result, param_names[idx], PARAM_NAME_LEN);	
	return true;
}

#endif
// clang-format on

#ifdef __cplusplus
}
#endif
#endif /* PARAMS_H_ */
