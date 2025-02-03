#ifndef INDICATORS_H_
#define INDICATORS_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "hardware.h"
#include "input.h"
#include "types.h"

void indicators_input_update(const InputEvents *events, Milliseconds now);

void indicators_output_draw(uint8_t out_channels_firing);

#ifdef __cplusplus
}
#endif
#endif /* INDICATORS_H_ */