#include "input.h"

#define RESET_PIN_THRESHOLD 100

static bool reset_active = false;

static int trig_in_value_previous = 0; 

static bool channel_pressed = false;

bool input_detect_rise_analog(int reset_in_value) {
  bool result = false;
  if ((!reset_active) && (reset_in_value >= RESET_PIN_THRESHOLD)) {
    reset_active = true;

    result = true;
  }
  if (reset_active && (reset_in_value < RESET_PIN_THRESHOLD)) {
    reset_active = false;
  }
  return result;
}

bool input_detect_rise_digital(int trig_in_value) {
  bool result = (trig_in_value > trig_in_value_previous);

  trig_in_value_previous = trig_in_value;
  
  return result;
}

EncoderIdx input_detect_enc_push(int channel_switch_val) {
  // Early return: No encoder is pushed
  if (channel_switch_val < 100) {
    channel_pressed = false;
    return ENCODER_NONE;
  }

  EncoderIdx enc_idx;
  if (channel_switch_val < 200) {
    enc_idx = ENCODER_2;
  } else if (channel_switch_val < 400) {
    enc_idx = ENCODER_1;
  } else {
    enc_idx = ENCODER_3;
  }

  // Early return: Encoder already registered as pushed
  if (channel_pressed) {
    return ENCODER_NONE;
  }
  
  channel_pressed = true;

  return enc_idx;
}
