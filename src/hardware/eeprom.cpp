#include "eeprom.h"

#include "logging.h"
#include "mode_params.h"

#if EEPROM_READ || EEPROM_WRITE
#include <EEPROM.h>
#endif

void eeprom_params_load(Params *params, Mode mode) {
	const uint8_t num_params = mode_num_params[mode];

	for (uint8_t idx = 0; idx < num_params; idx++) {
#if EEPROM_READ
		const Address addr = mode_param_address(mode, (ParamIdx)idx);
		params->values[idx] = EEPROM.read(addr);
#else
		params.values[idx] = 0;
#endif
		params->flags[idx] = PARAM_FLAGS_NONE;
	}

	params->len = num_params;
}

void eeprom_save_all_needing_write(Params *params, Mode mode) {
#if EEPROM_WRITE
	const uint8_t num_params = mode_num_params[mode];

	for (uint8_t idx = 0; idx < num_params; idx++) {
		const bool needs_write = param_flags_get(params, idx, PARAM_FLAG_NEEDS_WRITE);
		if (!needs_write) continue;

		param_flags_clear(params, idx, PARAM_FLAG_NEEDS_WRITE);

		const uint8_t val = params->values[idx];
		const Address addr = mode_param_address(mode, (ParamIdx)idx);
		EEPROM.write(addr, val);

		log_eeprom_write(mode, idx, addr, val);
	}
#endif
}