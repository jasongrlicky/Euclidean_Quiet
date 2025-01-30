#include "input.h"

#define RESET_PIN_THRESHOLD 100

static bool reset_active = false;

static int trig_in_value_previous = 0; 

static bool encoder_pushed = false;

bool input_events_contains_any_external(InputEvents *events) {
  bool result = (
    events->trig ||
    events->reset ||
    (events->enc_push != ENCODER_NONE) ||
    (events->enc_move[ENCODER_1] != 0) ||
    (events->enc_move[ENCODER_2] != 0) ||
    (events->enc_move[ENCODER_3] != 0)
  );
  return result;
}

bool input_detect_rise_reset(int reset_in_value) {
  bool above_threshold = (reset_in_value >= RESET_PIN_THRESHOLD);
  bool should_toggle = reset_active ^ above_threshold;
  reset_active ^= should_toggle;
  return (reset_active && should_toggle);
}

bool input_detect_rise_trig(int trig_in_value) {
  bool result = (trig_in_value > trig_in_value_previous);

  trig_in_value_previous = trig_in_value;
  
  return result;
}

EncoderIdx input_detect_enc_push(int channel_switch_val) {
  // Early return: No encoder is pushed
  if (channel_switch_val < 100) {
    encoder_pushed = false;
    return ENCODER_NONE;
  }

  // Early return: Encoder already registered as pushed
  if (encoder_pushed) {
    return ENCODER_NONE;
  }

  encoder_pushed = true;

  EncoderIdx enc_idx;
  if (channel_switch_val < 200) {
    enc_idx = ENCODER_2;
  } else if (channel_switch_val < 400) {
    enc_idx = ENCODER_1;
  } else {
    enc_idx = ENCODER_3;
  }
  return enc_idx;
}
