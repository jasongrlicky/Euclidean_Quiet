#ifndef LOGGING_H_
#define LOGGING_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "common/events.h"
#include "common/params.h"
#include "common/types.h"
#include "config.h"
#include "mode/mode.h"

void logging_init();
void log_cycle_time_begin();
void log_cycle_time_end(Milliseconds now);
void log_eeprom_write(Mode mode, ParamIdx idx, Address addr, uint8_t val);
void log_input_events(const InputEvents *events);
void log_all_modified_params(const Params *params, Mode mode);

#ifdef __cplusplus
}
#endif
#endif /* LOGGING_H_ */