#ifndef INPUT_H_
#define INPUT_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "hardware.h"
#include "types.h"

/// Record of any input events that were received this cycle
typedef struct InputEvents {
	/// Some encoders were rotated, indexed by `EncoderIdx`
	int16_t enc_move[NUM_ENCODERS];
	/// An encoder was pushed
	EncoderIdx enc_push;
	/// "Trig" input detected a rising edge
	bool trig;
	/// "Reset" input or button detected a rising edge
	bool reset;
	/// The internal clock generated a tick
	bool internal_clock_tick;
} InputEvents;

/// An instance of `InputEvents` which represents no events happening
const InputEvents INPUT_EVENTS_EMPTY = {
    .enc_move = {0, 0, 0},
    .enc_push = ENCODER_NONE,
    .trig = false,
    .reset = false,
    .internal_clock_tick = false,
};

/// Populates the passed-in struct with events observed since last cycle.
void input_update(InputEvents *events, Milliseconds now);

/// Returns true if `events` contains any externally-generated events
bool input_events_contains_any_external(const InputEvents *events);

#ifdef __cplusplus
}
#endif
#endif /* INPUT_H_ */
