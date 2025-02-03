#ifndef HARDWARE_H_
#define HARDWARE_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <pins_arduino.h>

/* HARDWARE CONSTANTS */

// Input pin definitions
#define PIN_IN_TRIG A0
#define PIN_IN_RESET A1
#define PIN_IN_CHANNEL_SWITCH A2

// Output pin definitions
#define PIN_OUT_CHANNEL_1 11
#define PIN_OUT_CHANNEL_2 12
#define PIN_OUT_CHANNEL_3 13
#define PIN_OUT_OFFBEAT 17
#define PIN_OUT_LED_DATA 2
#define PIN_OUT_LED_CLOCK 3
#define PIN_OUT_LED_SELECT 4

// Encoder pin definitions
#define PIN_ENC_1A 10
#define PIN_ENC_1B 9
#define PIN_ENC_2A 8
#define PIN_ENC_2B 7
#define PIN_ENC_3A 6
#define PIN_ENC_3B 5

// Number of encoders on the module
#define NUM_ENCODERS 3

/// Represents, optionally, one of the three encoders
enum EncoderIdx {
	ENCODER_1 = 0,
	ENCODER_2 = 1,
	ENCODER_3 = 2,
	ENCODER_NONE = 4,
};

// LED Matrix address
#define LED_ADDR 0

// Number of rows in LED Matrix
#define LED_ROWS 8
// Number of columns in LED Matrix
#define LED_COLUMNS 8

#ifdef __cplusplus
}
#endif
#endif /* HARDWARE_H_ */
