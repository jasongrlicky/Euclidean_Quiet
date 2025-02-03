#ifndef EVENTS_H_
#define EVENTS_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "hardware/properties.h"

#include <stdbool.h>
#include <stdint.h>

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
extern const InputEvents INPUT_EVENTS_EMPTY;

/// Returns true if `events` contains any externally-generated events
bool input_events_contains_any_external(const InputEvents *events);

#ifdef __cplusplus
}
#endif
#endif /* EVENTS_H_ */