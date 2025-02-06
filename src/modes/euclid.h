#ifndef EUCLID_H_
#define EUCLID_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/* CONSTANTS */

#define NUM_CHANNELS 3

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

/// A parameter of the Euclidean rhythm generator
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

/// State of the entire Euclidean module
typedef struct EuclideanState {
	/// State for each of this module's channels, indexed by `Channel` enum.
	EuclideanChannelState channels[NUM_CHANNELS];
	bool sequencer_running;
} EuclideanState;

/* GLOBALS */

extern EuclideanState euclidean_state;

/* EXTERNAL */

/// Wrap the provided value as an occupied optional
EuclideanParamOpt euclidean_param_opt(EuclideanParam inner);

#ifdef __cplusplus
}
#endif
#endif /* EUCLID_H_ */