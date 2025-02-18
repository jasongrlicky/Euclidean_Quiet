#ifndef EUCLID_H_
#define EUCLID_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "common/events.h"
#include "common/timeout.h"
#include "common/types.h"
#include "params.h"

#include <stdbool.h>
#include <stdint.h>

/* CONSTANTS */

#define NUM_CHANNELS 3
#define EUCLID_PARAMS_PER_CHANNEL 3

// Bounds for three channel parameters
// N: Beat Length
#define BEAT_LENGTH_MIN 1
#define BEAT_LENGTH_MAX 16
#define BEAT_LENGTH_DEFAULT 16
// K: Density
#define BEAT_DENSITY_MIN 0
#define BEAT_DENSITY_MAX 16
#define BEAT_DENSITY_DEFAULT 4
// O: Offset
#define BEAT_OFFSET_MIN 0
#define BEAT_OFFSET_MAX 15
#define BEAT_OFFSET_DEFAULT 0
// Sequencer position
#define BEAT_POSITION_MAX 15
#define BEAT_POSITION_DEFAULT 0

/* DATA STRUCTURES */

/// The kind of a parameter for a channel of the Euclidean rhythm generator
typedef enum EuclideanParam {
	EUCLIDEAN_PARAM_LENGTH,
	EUCLIDEAN_PARAM_DENSITY,
	EUCLIDEAN_PARAM_OFFSET,
} EuclideanParam;

/// Euclidean parameter that is wrapped as an optional value
typedef struct EuclideanParamOpt {
	EuclideanParam inner;
	bool valid;
} EuclideanParamOpt;

static const EuclideanParamOpt EUCLIDEAN_PARAM_OPT_NONE = {.inner = EUCLIDEAN_PARAM_LENGTH, .valid = false};

#if !PARAM_TABLES
/// State of the Euclidean rhythm generator and sequencer for a single channel
typedef struct EuclideanChannelState {
	/// Number of steps in the Euclidean rhythm, 1-16
	uint8_t length;
	/// Number of active steps in the Euclidean rhythm, 1-16
	uint8_t density;
	/// Number of steps to rotate the Euclidean rhythm to the right, 1-16
	uint8_t offset;
	/// Step index representing the playhead position for this channel's sequencer, 0-15
	uint8_t position;
} EuclideanChannelState;
#endif

/// State of the entire Euclidean module
typedef struct EuclideanState {
#if PARAM_TABLES
	/// Step index representing the playhead position for for each of this mode's
	/// channels, indexed by `Channel` enum. Valid values are `0` to `15`.
	uint8_t channel_positions[NUM_CHANNELS];
#else
	/// State for each of this module's channels, indexed by `Channel` enum.
	EuclideanChannelState channels[NUM_CHANNELS];
#endif
	bool sequencer_running;
} EuclideanState;

typedef struct AdjustmentDisplayState {
	/// Which channel is currently showing its adjustment display. Only one
	/// adjustment display can be visible at a time.
	Channel channel;
	/// The parameter that is being displayed in the adjustment display.
	EuclideanParam parameter;
	/// Is the adjustment display showing currently
	bool visible;
} AdjustmentDisplayState;

/* GLOBALS */

extern EuclideanState euclidean_state;
extern uint16_t generated_rhythms[NUM_CHANNELS];
extern AdjustmentDisplayState adjustment_display_state;
extern TimeoutOnce playhead_flash_timeout;
extern ParamsRuntime params;

/* EXTERNAL */

/// Return the `ParamIdx` for a given a channel and param kind
inline ParamIdx euclid_param_idx(Channel channel, EuclideanParam kind) {
	return (ParamIdx)((channel * EUCLID_PARAMS_PER_CHANNEL) + kind);
}

/// Get the value of the specified param. Do not use if Euclidean is not the
/// active mode.
inline uint8_t euclid_param_get(Channel channel, EuclideanParam kind) {
	ParamIdx idx = euclid_param_idx(channel, kind);
	return params.values[idx];
}

/// Set the value of the specified param. Do not use if Euclidean is not the
/// active mode.
inline void euclid_param_set(Channel channel, EuclideanParam kind, uint8_t val) {
	ParamIdx idx = euclid_param_idx(channel, kind);
	params.values[idx] = val;
}

/// Do not use if Euclidean is not the active mode.
inline uint8_t euclid_param_get_length(Channel channel) {
	return euclid_param_get(channel, EUCLIDEAN_PARAM_LENGTH);
}

/// Do not use if Euclidean is not the active mode.
inline uint8_t euclid_param_get_density(Channel channel) {
	return euclid_param_get(channel, EUCLIDEAN_PARAM_DENSITY);
}

/// Do not use if Euclidean is not the active mode.
inline uint8_t euclid_param_get_offset(Channel channel) {
	return euclid_param_get(channel, EUCLIDEAN_PARAM_OFFSET);
}

/// Wrap the provided value as an occupied optional
EuclideanParamOpt euclidean_param_opt(EuclideanParam inner);

// Returns bitflags storing which output channels will fire this cycle, indexed
// by `OutputChannel`.
uint8_t euclid_update(const InputEvents *events);

void euclid_draw_channels(void);

#if !PARAM_TABLES
/// Keep the data in the state in bounds. Bounds excursions can happen when
/// loading from the EEPROM.
void euclid_validate_state(EuclideanState *s);
#endif

#ifdef __cplusplus
}
#endif
#endif /* EUCLID_H_ */