#ifndef EUCLID_H_
#define EUCLID_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "common/events.h"
#include "common/params.h"
#include "common/timeout.h"
#include "common/types.h"
#include "ui/framebuffer.h"

#define NUM_CHANNELS 3

typedef struct EuclidSequencerState {
	/// Step index representing the playhead position for for each of this mode's
	/// channels, indexed by `Channel` enum. Valid values are `0` to `15`.
	uint8_t positions[NUM_CHANNELS];
	bool running;
} EuclidSequencerState;

/// Only one adjustment display can be visible at a time, and in this mode, only
/// the length parameter shows an adjustment display.
typedef struct EuclidAdjustmentDisplayState {
	Timeout timeout;
	/// Which channel is currently showing its adjustment display.
	Channel channel;
	bool visible;
} EuclidAdjustmentDisplayState;

typedef struct EuclidOutputPulseState {
	/// Timeout duration is the output pulse length, set based on the time since last trigger
	TimeoutOnce timeout;
	Milliseconds last_clock_or_reset;
} EuclidOutputPulseState;

typedef struct EuclidPlayheadState {
	// Tracks the playhead flash itself
	TimeoutOnce flash_timeout;
	// Track the time since the playhead has moved so we can make it flash in its idle loop
	Timeout idle_timeout;
	// Loop for making the playhead flash periodically after it is idle
	Timeout idle_loop_timeout;
} EuclidPlayheadState;

/// State of the entire Euclidean rhythm generator mode
typedef struct EuclidState {
	/// The sequencer channel that is currently selected
	Channel active_channel;
	/// Stores each generated Euclidean rhythm as 16 bits. Indexed by channel number.
	uint16_t generated_rhythms[NUM_CHANNELS];
	EuclidSequencerState sequencer;
	EuclidAdjustmentDisplayState adjustment_display;
	EuclidOutputPulseState output_pulse;
	EuclidPlayheadState playhead;
} EuclidState;

void euclid_params_validate(Params *params);
void euclid_init(EuclidState *state, const Params *params, Framebuffer *fb);
void euclid_update(EuclidState *state, Params *params, Framebuffer *fb, const InputEvents *events,
                   Milliseconds now);

#ifdef __cplusplus
}
#endif
#endif /* EUCLID_H_ */