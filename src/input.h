#ifndef INPUT_H_
#define INPUT_H_

#include <stdint.h>

#include "hardware.h"

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
  .enc_move = { 0, 0, 0 },
  .enc_push = ENCODER_NONE,
  .trig = false,
  .reset = false,
  .internal_clock_tick = false,
};

/// Returns true if `events` contains any externally-generated events
bool input_events_contains_any_external(InputEvents *events);

/// @brief Detects rising edge for reset input
/// @param reset_in_value Raw reading from reset input pin
/// @return `true` if a rising edge was detected, `false` otherwise
bool input_detect_rise_reset(int reset_in_value);

/// @brief Detects rising edge for trigger input
/// @param trig_in_value Raw reading from trig input pin
/// @return `true` if a rising edge was detected, `false` otherwise
bool input_detect_rise_trig(int trig_in_value);

/// @brief Detects initial event of an encoder being pushed
/// @param channel_switch_val Raw reading from channel switch pin
/// @return `true` if an encoder was pushed this cycle, `false` otherwise
EncoderIdx input_detect_enc_push(int channel_switch_val);

#endif /* INPUT_H_ */
