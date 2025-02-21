#ifndef MODE_PARAMS_H_
#define MODE_PARAMS_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "mode.h"
#include "params.h"

#define EUCLID_NUM_PARAMS 9
/// How many params this mode has. Indexed by the `Mode` enum.
const uint8_t mode_num_params[NUM_MODES] = {
    EUCLID_NUM_PARAMS, // EUCLID
};

void mode_params_validate(Params *params, Mode mode);

Address mode_param_address(Mode mode, ParamIdx idx);

/// Clear `PARAM_FLAG_MODIFIED` for all parameters
void mode_params_flags_clear_all_modified(Params *params, Mode mode);

#if LOGGING_ENABLED

#define PARAM_NAME_LEN 3

/// @brief Retrieve the name for the specified parameter and store that name in
/// the `result` null-terminated string.
/// @param result A `char` array that can hold at least `PARAM_NAME_LEN`
/// elements. Will have the parameter name stored in it as a null-terminated
/// string, or a placeholder if the param name can't be found.
/// @param mode The mode for the parameter
/// @param idx The index of the parameter in the parameter tables
void mode_param_name(char *result, Mode mode, ParamIdx idx);

#endif

#ifdef __cplusplus
}
#endif
#endif /* MODE_PARAMS_H_ */
