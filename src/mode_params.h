#ifndef MODE_PARAMS_H_
#define MODE_PARAMS_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "mode.h"
#include "params.h"

#define EUCLID_NUM_PARAMS 9
/// How many params this mode has. Indexed by the `Mode` enum.
static const uint8_t mode_num_params[NUM_MODES] = {
    EUCLID_NUM_PARAMS, // EUCLID
};

void mode_params_validate(Params *params, Mode mode);

Address mode_param_address(Mode mode, ParamIdx idx);

/// Clear `PARAM_FLAG_MODIFIED` for all parameters
void mode_params_flags_clear_all_modified(Params *params, Mode mode);

#ifdef __cplusplus
}
#endif
#endif /* MODE_PARAMS_H_ */
