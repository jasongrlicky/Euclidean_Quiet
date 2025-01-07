#ifndef DEFINES_H_INCLUDE
#define DEFINES_H_INCLUDE

/* HARDWARE CONSTANTS */

// Input pin definitions
#define PIN_IN_TRIG A0
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

// LED Matrix address
#define LED_ADDR 0

/* SOFTWARE CONSTANTS */

#define NUM_CHANNELS 3
// Bounds for three channel parameters
// N: Beat Length
#define BEAT_LENGTH_MAX 16
#define BEAT_LENGTH_MIN 1
// K: Density
#define BEAT_DENSITY_MIN 0
// O: Offset
#define BEAT_OFFSET_MIN 0

#endif /* DEFINES_H_INCLUDE */ 
