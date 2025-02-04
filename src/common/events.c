#include "events.h"

const InputEvents INPUT_EVENTS_EMPTY = {
    .enc_move = {0, 0, 0},
    .enc_push = ENCODER_NONE,
    .trig = false,
    .reset = false,
    .internal_clock_tick = false,
};

// cppcheck-suppress unusedFunction
bool input_events_contains_any_external(const InputEvents *events) {
	if (!events) return false;

	// clang-format off
	bool result = (events->trig || 
                   events->reset || 
                  (events->enc_push != ENCODER_NONE) ||
	              (events->enc_move[ENCODER_1] != 0) || 
                  (events->enc_move[ENCODER_2] != 0) ||
	              (events->enc_move[ENCODER_3] != 0));
	// clang-format on
	return result;
}
