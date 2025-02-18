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

/// State of the entire Euclidean module
typedef struct EuclideanState {
	/// Step index representing the playhead position for for each of this mode's
	/// channels, indexed by `Channel` enum. Valid values are `0` to `15`.
	uint8_t channel_positions[NUM_CHANNELS];
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
extern Params params;

/* EXTERNAL */

/// Return the `ParamIdx` for a given a channel and param kind
inline ParamIdx euclid_param_idx(Channel channel, EuclideanParam kind) {
	return (ParamIdx)((channel * EUCLID_PARAMS_PER_CHANNEL) + kind);
}

inline uint8_t euclid_param_get(const Params *params, Channel channel, EuclideanParam kind) {
	ParamIdx idx = euclid_param_idx(channel, kind);
	return params->values[idx];
}

/// Set the value of the specified param. Do not use if Euclidean is not the
/// active mode.
inline void euclid_param_set(Params *params, Channel channel, EuclideanParam kind, uint8_t val) {
	ParamIdx idx = euclid_param_idx(channel, kind);
	params->values[idx] = val;
}

/// Do not use if Euclidean is not the active mode.
inline uint8_t euclid_get_length(const Params *params, Channel channel) {
	return euclid_param_get(params, channel, EUCLIDEAN_PARAM_LENGTH);
}

/// Do not use if Euclidean is not the active mode.
inline uint8_t euclid_get_density(const Params *params, Channel channel) {
	return euclid_param_get(params, channel, EUCLIDEAN_PARAM_DENSITY);
}

/// Do not use if Euclidean is not the active mode.
inline uint8_t euclid_get_offset(const Params *params, Channel channel) {
	return euclid_param_get(params, channel, EUCLIDEAN_PARAM_OFFSET);
}

void euclid_params_validate(Params *params);

/// Wrap the provided value as an occupied optional
EuclideanParamOpt euclidean_param_opt(EuclideanParam inner);

// Returns bitflags storing which output channels will fire this cycle, indexed
// by `OutputChannel`.
uint8_t euclid_update(const InputEvents *events);

void euclid_draw_channels(void);

#ifdef __cplusplus
}
#endif
#endif /* EUCLID_H_ */