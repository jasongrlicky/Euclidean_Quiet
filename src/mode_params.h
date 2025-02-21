#ifndef MODE_PARAMS_H_
#define MODE_PARAMS_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "mode.h"
#include "params.h"

void mode_params_validate(Params *params, Mode mode);

Address mode_param_address(Mode mode, ParamIdx idx);

/// Clear `PARAM_FLAG_MODIFIED` for all parameters
void mode_params_flags_clear_all_modified(Params *params, Mode mode);

#ifdef __cplusplus
}
#endif
#endif /* MODE_PARAMS_H_ */
