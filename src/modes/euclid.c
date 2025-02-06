#include "euclid.h"

/* GLOBALS */

// clang-format off
EuclideanState euclidean_state = {
  .channels = {
    { .length = BEAT_LENGTH_DEFAULT, .density = BEAT_DENSITY_DEFAULT, .offset = BEAT_OFFSET_DEFAULT, .position = 0 },
    { .length = BEAT_LENGTH_DEFAULT, .density = BEAT_DENSITY_DEFAULT, .offset = BEAT_OFFSET_DEFAULT, .position = 0 },
    { .length = BEAT_LENGTH_DEFAULT, .density = BEAT_DENSITY_DEFAULT, .offset = BEAT_OFFSET_DEFAULT, .position = 0 }
  },
  .sequencer_running = false,
};
// clang-format on

/* EXTERNAL */

EuclideanParamOpt euclidean_param_opt(EuclideanParam inner) {
	return (EuclideanParamOpt){.inner = inner, .valid = true};
}