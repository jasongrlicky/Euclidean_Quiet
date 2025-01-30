#include "input.h"

#define RESET_PIN_THRESHOLD 100

static bool reset_active = false;

static int trig_in_value_previous = 0; 

static unsigned long channel_pressed_counter = 0;

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
  bool enc_pushed = false;
  EncoderIdx enc_idx = ENCODER_NONE;
  if (channel_switch_val < 100) {
    // Nothing pushed
    channel_pressed_counter = 0;
  } else {
    enc_pushed = true;
    channel_pressed_counter++;

    if (channel_switch_val < 200) {
     enc_idx = ENCODER_2;
    } else if (channel_switch_val < 400) {
      enc_idx = ENCODER_1;
    } else {
      enc_idx = ENCODER_3;
    }
  }

  EncoderIdx result = ENCODER_NONE;
  if (enc_pushed && (channel_pressed_counter <= 1)) {
    result = enc_idx;
  }

  return result;
}
