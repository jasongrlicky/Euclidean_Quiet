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
  bool enc_pushed;
  EncoderIdx enc_idx;
  if (channel_switch_val < 100) {
    // Nothing pushed
    enc_pushed = false;
    enc_idx = ENCODER_NONE;
    channel_pressed_counter = 0;
  } else if (channel_switch_val < 200) {
    // Density pushed
    enc_pushed = true;
    enc_idx = ENCODER_2;
    channel_pressed_counter++;
  } else if (channel_switch_val < 400) {
    // Length pushed
    enc_pushed = true;
    enc_idx = ENCODER_1;
    channel_pressed_counter++;
  } else {
    // Offset pushed
    enc_pushed = true;
    enc_idx = ENCODER_3;
    channel_pressed_counter++;
  }

  EncoderIdx result = ENCODER_NONE;
  if (enc_pushed && (channel_pressed_counter <= 1)) {
    result = enc_idx;
  }

  return result;
}
