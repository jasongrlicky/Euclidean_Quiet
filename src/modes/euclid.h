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
typedef enum EuclidParam {
	EUCLID_PARAM_LENGTH,
	EUCLID_PARAM_DENSITY,
	EUCLID_PARAM_OFFSET,
} EuclidParam;

/// Parameter for the Euclidean rhythm generator mode that is wrapped as an optional value
typedef struct EuclidParamOpt {
	EuclidParam inner;
	bool valid;
} EuclidParamOpt;

static const EuclidParamOpt EUCLID_PARAM_OPT_NONE = {.inner = EUCLID_PARAM_LENGTH, .valid = false};

/// State of the entire Euclidean rhythm generator mode
typedef struct EuclidState {
	/// The sequencer channel that is currently selected
	Channel active_channel;
	/// Step index representing the playhead position for for each of this mode's
	/// channels, indexed by `Channel` enum. Valid values are `0` to `15`.
	uint8_t sequencer_positions[NUM_CHANNELS];
	bool sequencer_running;
} EuclidState;

typedef struct AdjustmentDisplayState {
	/// Which channel is currently showing its adjustment display. Only one
	/// adjustment display can be visible at a time.
	Channel channel;
	/// The parameter that is being displayed in the adjustment display.
	EuclidParam parameter;
	/// Is the adjustment display showing currently
	bool visible;
} AdjustmentDisplayState;

/* GLOBALS */

extern EuclidState euclid_state;
extern uint16_t generated_rhythms[NUM_CHANNELS];
extern AdjustmentDisplayState adjustment_display_state;
extern TimeoutOnce playhead_flash_timeout;
extern Params params;

/* EXTERNAL */

/// Return the `ParamIdx` for a given a channel and param kind
inline ParamIdx euclid_param_idx(Channel channel, EuclidParam kind) {
	return (ParamIdx)((channel * EUCLID_PARAMS_PER_CHANNEL) + kind);
}

inline uint8_t euclid_param_get(const Params *params, Channel channel, EuclidParam kind) {
	ParamIdx idx = euclid_param_idx(channel, kind);
	return params->values[idx];
}

/// Set the value of the specified param. Do not use if Euclidean is not the
/// active mode.
inline void euclid_param_set(Params *params, Channel channel, EuclidParam kind, uint8_t val) {
	ParamIdx idx = euclid_param_idx(channel, kind);
	params->values[idx] = val;
}

/// Do not use if Euclidean is not the active mode.
inline uint8_t euclid_get_length(const Params *params, Channel channel) {
	return euclid_param_get(params, channel, EUCLID_PARAM_LENGTH);
}

/// Do not use if Euclidean is not the active mode.
inline uint8_t euclid_get_density(const Params *params, Channel channel) {
	return euclid_param_get(params, channel, EUCLID_PARAM_DENSITY);
}

/// Do not use if Euclidean is not the active mode.
inline uint8_t euclid_get_offset(const Params *params, Channel channel) {
	return euclid_param_get(params, channel, EUCLID_PARAM_OFFSET);
}

void euclid_params_validate(Params *params);

/// Wrap the provided value as an occupied optional
EuclidParamOpt euclid_param_opt(EuclidParam inner);

/// Given an `EncoderIdx`, return the associated Euclidean parameter
EuclidParamOpt euclid_param_for_encoder(EncoderIdx enc_idx);

void euclid_init(void);
void euclid_update(const InputEvents *events, Milliseconds now);
void euclid_handle_encoder_push(EncoderIdx enc_idx);
EuclidParamOpt euclid_handle_encoder_move(const int16_t *enc_move);

// Returns bitflags storing which output channels will fire this cycle, indexed
// by `OutputChannel`.
uint8_t euclid_update_sequencers(const InputEvents *events);

void euclid_draw_channels(void);

#ifdef __cplusplus
}
#endif
#endif /* EUCLID_H_ */