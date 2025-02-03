#ifndef OUTPUT_H_
#define OUTPUT_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "common/hardware/properties.h"

#include <stdbool.h>

#define output_set_high(channel) (output_set(channel, true))
#define output_set_low(channel) (output_set(channel, false))
/// Send output signals on the actual hardware
void output_set(OutputChannel channel, bool value);

void output_clear_all(void);

#ifdef __cplusplus
}
#endif
#endif /* OUTPUT_H_ */