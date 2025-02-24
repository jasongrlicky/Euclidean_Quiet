#ifndef MODE_H_
#define MODE_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "common/events.h"
#include "common/params.h"
#include "common/types.h"
#include "config.h"
#include "ui/framebuffer.h"

#define NUM_MODES 1
typedef enum Mode {
	MODE_EUCLID,
} Mode;

void mode_init(Params *params, Framebuffer *fb, Mode mode);
void mode_update(Params *params, Framebuffer *fb, Mode mode, const InputEvents *events, Milliseconds now);

#define EUCLID_NUM_PARAMS 9
/// How many params this mode has. Indexed by the `Mode` enum.
const uint8_t mode_num_params[NUM_MODES] = {
    EUCLID_NUM_PARAMS, // EUCLID
};

void mode_params_validate(Params *params, Mode mode);

Address mode_param_address(Mode mode, ParamIdx idx);

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
#endif /* MODE_H_ */
