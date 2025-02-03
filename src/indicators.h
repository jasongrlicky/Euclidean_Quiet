#ifndef INDICATORS_H_
#define INDICATORS_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "hardware.h"

void draw_output_indicators(uint8_t out_channels_firing);

#ifdef __cplusplus
}
#endif
#endif /* INDICATORS_H_ */