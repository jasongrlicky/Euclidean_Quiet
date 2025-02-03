#include "input.h"

#include <Arduino.h>
#include <Encoder.h>

#include "config.h"
#include "timeout.h"

/* CONFIG */

#define RESET_PIN_THRESHOLD 100

/* STATE */

static bool reset_active = false;

static int trig_in_value_previous = 0;

static bool encoder_pushed = false;

// Initialize objects for reading encoders
// (from the Encoder.h library)
static Encoder encoders[NUM_ENCODERS] = {
    // Length  (Ch1) / N
    Encoder(PIN_ENC_2B, PIN_ENC_2A),
    // Density (Ch2) / K
    Encoder(PIN_ENC_1B, PIN_ENC_1A),
    // Offset  (Ch3) / O
    Encoder(PIN_ENC_3B, PIN_ENC_3A),
};

static Timeout encoder_read_timeout = {.duration = READ_DELAY};

/* DECLARATIONS */

/// @brief Detects rising edge for reset input
/// @param reset_in_value Raw reading from reset input pin
/// @return `true` if a rising edge was detected, `false` otherwise
static bool detect_rise_reset(int reset_in_value);

/// @brief Detects rising edge for trigger input
/// @param trig_in_value Raw reading from trig input pin
/// @return `true` if a rising edge was detected, `false` otherwise
static bool detect_rise_trig(int trig_in_value);

/// @brief Detects initial event of an encoder being pushed
/// @param channel_switch_val Raw reading from channel switch pin
/// @return `true` if an encoder was pushed this cycle, `false` otherwise
static EncoderIdx detect_enc_push(int channel_switch_val);

/// @brief Detect encoder movement
/// @param enc Encoder object that abstracts reading encoders
/// @return +1, 0 or -1 to represent the direction the encoder was turned.
static int encoder_read(Encoder &enc);

/* EXTERNAL */

// cppcheck-suppress unusedFunction
void input_update(InputEvents *events, Milliseconds now) {
	// Reset Input & Button
	int reset_in_value = analogRead(PIN_IN_RESET);
	events->reset = detect_rise_reset(reset_in_value);

	// Trig Input
	int trig_in_value = digitalRead(PIN_IN_TRIG);
	events->trig = detect_rise_trig(trig_in_value);

	// Encoder Movement
	bool move_detected = false;
	if (timeout_fired(&encoder_read_timeout, now)) {
		for (uint8_t enc_idx = 0; enc_idx < NUM_ENCODERS; enc_idx++) {
			int val = encoder_read(encoders[enc_idx]);
			events->enc_move[enc_idx] = val;
			move_detected |= (val != 0);
		}
	}
	if (move_detected) {
		timeout_reset(&encoder_read_timeout, now);
	}

	// Encoder Pushes
	int channel_switch_val = analogRead(PIN_IN_CHANNEL_SWITCH);
	events->enc_push = detect_enc_push(channel_switch_val);
}

/* INTERNAL */

static bool detect_rise_reset(int reset_in_value) {
	bool above_threshold = (reset_in_value >= RESET_PIN_THRESHOLD);
	bool should_toggle = reset_active ^ above_threshold;
	reset_active ^= should_toggle;
	return (reset_active && should_toggle);
}

static bool detect_rise_trig(int trig_in_value) {
	bool result = (trig_in_value > trig_in_value_previous);

	trig_in_value_previous = trig_in_value;

	return result;
}

static EncoderIdx detect_enc_push(int channel_switch_val) {
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

static int encoder_read(Encoder &enc) {
	int result = 0;
	int32_t value_read = enc.read();
	if (value_read == 0) {
		enc.write(0);
		result = 0;
	} else if (value_read < -2) {
		result = -1;
		enc.write(0);
	} else if (value_read > 2) {
		result = 1;
		enc.write(0);
	}
	return result;
}
