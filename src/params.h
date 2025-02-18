#ifndef PARAMS_H_
#define PARAMS_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "common/types.h"
#include "config.h"

#include <stdint.h>
#include <string.h>

/* Parameters are variables (stored as individual bytes) which are
 * programmatically tracked, read from and written to EEPROM, and logged. Each
 * mode has multiple parameters, in addition to its ephemeral state.
 */

#define NUM_MODES 1
typedef enum Mode {
	MODE_EUCLID,
} Mode;

#define EUCLID_NUM_PARAMS 9

/// How many params this mode has. Indexed by the `Mode` enum.
static const uint8_t mode_num_params[NUM_MODES] = {
    EUCLID_NUM_PARAMS, // EUCLID
};

#define PARAM_FLAGS_NONE 0x0
#define PARAM_FLAG_MODIFIED 0x1
#define PARAM_FLAG_NEEDS_WRITE 0x2

/// Maximum size of `ParamsRuntime`'s tables. Must be large enough to store the
/// `ParamId` type for any mode.
#define PARAMS_RUNTIME_MAX 9

/// Parameter properties which need to be modified at runtime. Each table has
/// the same length (`.len`), and they are indexed by a mode's associated
/// `ParamId` type.
typedef struct ParamsRuntime {
	/// Number of elements in tables
	uint8_t len;
	/// List of parameter values of length `.len`. The values are always assumed to be in bounds.
	uint8_t values[PARAMS_RUNTIME_MAX];
	/// List of parameter properties, stored as bitflags, of length `.len`. Bitflags are indexed
	/// via `PARAM_FLAG_*` defines.
	uint8_t flags[PARAMS_RUNTIME_MAX];
} ParamsRuntime;

/// Stores the runtime information of the active mode's parameters.
ParamsRuntime params;

/*
Original EEPROM Schema:
Channel 1: length = 1 density = 2 offset = 7
Channel 2: length = 3 density = 4 offset = 8
Channel 3: length = 5 density = 6 offset = 9
*/

/// EEPROM addresses for Euclidean mode params. The order is this way for
/// backwards-compatibility with the original Sebsongs Euclidean firmware.
static const Address euclid_param_addresses[EUCLID_NUM_PARAMS] = {1, 2, 7, 3, 4, 8, 5, 6, 9};

Address param_address(Mode mode, ParamIdx idx);

// clang-format off
#if LOGGING_ENABLED

#define PARAM_NAME_LEN 3
/// Table of parameter names for logging in the Euclid mode
static const char euclid_param_names[EUCLID_NUM_PARAMS][PARAM_NAME_LEN] = {
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

/// @brief Calculate or look up the name for the specified parameter and store
/// that name in the `result` null-terminated string.
/// @param result A `char` array that can hold at least `PARAM_NAME_LEN` elements.
/// @param mode The mode for the parameter
/// @param idx The index of the parameter in the parameter tables
/// @return `true` if the name for the parameter was found 
bool param_name(char *result, Mode mode, ParamIdx idx);

#endif
// clang-format on

#ifdef __cplusplus
}
#endif
#endif /* PARAMS_H_ */
