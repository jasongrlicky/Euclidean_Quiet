#ifndef OUTPUT_H_INCLUDE
#define OUTPUT_H_INCLUDE

#include <Arduino.h>

#include "defines.h"
#include "types.h"

#define output_set_high(channel) (output_set(channel, true))
#define output_set_low(channel) (output_set(channel, false))
void output_set(Channel channel, bool value);

void output_clear_all(void);

bool output_any_active(void);

#endif /* OUTPUT_H_INCLUDE */ 