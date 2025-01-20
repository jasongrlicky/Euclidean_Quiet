#ifndef OUTPUT_H_
#define OUTPUT_H_

#include <Arduino.h>

// Indices for individual output channels
typedef enum OutputChannel {
  OUTPUT_CHANNEL_1 = 0,
  OUTPUT_CHANNEL_2 = 1,
  OUTPUT_CHANNEL_3 = 2,
  OUTPUT_CHANNEL_OFFBEAT = 3
} OutputChannel;

#define OUTPUT_NUM_CHANNELS 4

#define output_set_high(channel) (output_set(channel, true))
#define output_set_low(channel) (output_set(channel, false))
void output_set(OutputChannel channel, bool value);

void output_clear_all(void);

bool output_any_active(void);

#endif /* OUTPUT_H_ */ 