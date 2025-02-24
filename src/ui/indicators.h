#ifndef INDICATORS_H_
#define INDICATORS_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "common/events.h"
#include "common/types.h"
#include "framebuffer.h"

#include <stdint.h>

void indicators_input_update(Framebuffer *fb, const InputEvents *events, Milliseconds now);

/// Draw output indicators for latching outputs - ones that stay lit until they
/// are specifically unlit on the next clock cycle.
/// @param out_channels_firing Bitflags storing which output channels will fire
/// this cycle, indexed by `OutputChannel`.
void indicators_output_draw_latching(Framebuffer *fb, uint8_t out_channels_firing);

#ifdef __cplusplus
}
#endif
#endif /* INDICATORS_H_ */