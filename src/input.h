#ifndef INPUT_H_
#define INPUT_H_

#include "hardware.h"

/// @brief Detects rising edge for reset input
/// @param reset_in_value Raw reading from reset input pin
/// @return `true` if a rising edge was detected, `false` otherwise
bool input_detect_reset(int reset_in_value);

/// @brief Detects rising edge for trigger input
/// @param trig_in_value Raw reading from trig input pin
/// @return `true` if a rising edge was detected, `false` otherwise
bool input_detect_trig(int trig_in_value);

/// @brief Detects an encoder being pushed
/// @param channel_switch_val Raw reading from channel switch pin
/// @return `true` if an encoder was pushed, `false` otherwise
EncoderIdx input_detect_enc_push(int channel_switch_val);

#endif /* INPUT_H_ */
