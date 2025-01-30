#ifndef INPUT_H_
#define INPUT_H_

#include "hardware.h"

bool input_detect_reset(int reset_in_value);
bool input_detect_trig(int trig_in_value);
EncoderIdx input_detect_enc_push(int channel_switch_val);

#endif /* INPUT_H_ */
