#ifndef EEPROM_H_
#define EEPROM_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "mode/mode.h"
#include "params.h"

/// Load state for the given mode into `params`.
void eeprom_params_load(Params *params, Mode mode);
void eeprom_save_all_needing_write(Params *params, Mode mode);

#ifdef __cplusplus
}
#endif
#endif /* EEPROM_H_ */
