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

/// Maximum size of `Params`'s tables. Must be large enough to store the
/// `ParamId` type for any mode.
#define PARAMS_MAX 9

/// Parameter properties which need to be modified at runtime. Each table has
/// the same length (`.len`), and they are indexed by a mode's associated
/// `ParamId` type.
typedef struct Params {
	/// Number of elements in tables
	uint8_t len;
	/// List of parameter values of length `.len`. The values are always assumed to be in bounds.
	uint8_t values[PARAMS_MAX];
	/// List of parameter properties, stored as bitflags, of length `.len`. Bitflags are indexed
	/// via `PARAM_FLAG_*` defines.
	uint8_t flags[PARAMS_MAX];
} Params;

/// Stores the runtime-modifiable information for the active mode's parameters.
/// Static information, such as addresses or names, is stored separately.
extern Params params;

/// Set the param referenced by `idx` to `value`, and set its flags to indicate
/// that it has been modified and needs to be written to the EEPROM.
void param_and_flags_set(Params *params, ParamIdx idx, uint8_t value);
/// Read the bits specified in `mask`
uint8_t param_flags_get(const Params *params, ParamIdx idx, uint8_t mask);
/// Set the bits specified in `mask` to 1, leaving the others untouched
void param_flags_set(Params *params, ParamIdx idx, uint8_t mask);
/// Clear the bits specified in `mask` to 0, leaving the others untouched
void param_flags_clear(Params *params, ParamIdx idx, uint8_t mask);
/// Clear `PARAM_FLAG_MODIFIED` for all parameters
void param_flags_clear_all_modified(Params *params, Mode mode);

Address param_address(Mode mode, ParamIdx idx);

#if LOGGING_ENABLED

#define PARAM_NAME_LEN 3

/// @brief Retrieve the name for the specified parameter and store that name in
/// the `result` null-terminated string.
/// @param result A `char` array that can hold at least `PARAM_NAME_LEN`
/// elements. Will have the parameter name stored in it as a null-terminated
/// string, or a placeholder if the param name can't be found.
/// @param mode The mode for the parameter
/// @param idx The index of the parameter in the parameter tables
void param_name(char *result, Mode mode, ParamIdx idx);

#endif

#ifdef __cplusplus
}
#endif
#endif /* PARAMS_H_ */
