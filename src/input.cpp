#include "input.h"

#define RESET_PIN_THRESHOLD 100

static bool reset_active = false;

static int trig_in_value_previous = 0; 

static unsigned long channelPressedCounter = 0;

bool input_detect_reset(int reset_in_value) {
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

bool input_detect_trig(int trig_in_value) {
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
    channelPressedCounter = 0;
  } else if (channel_switch_val < 200) {
    // Density pushed
    enc_pushed = true;
    enc_idx = ENCODER_2;
    channelPressedCounter++;
  } else if (channel_switch_val < 400) {
    // Length pushed
    enc_pushed = true;
    enc_idx = ENCODER_1;
    channelPressedCounter++;
  } else {
    // Offset pushed
    enc_pushed = true;
    enc_idx = ENCODER_3;
    channelPressedCounter++;
  }

  EncoderIdx result = ENCODER_NONE;
  if (enc_pushed && (channelPressedCounter <= 1)) {
    result = enc_idx;
  }

  return result;
}
