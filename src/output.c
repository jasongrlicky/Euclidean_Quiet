#include "output.h"

#include <Arduino.h>

#include "hardware.h"

static inline uint8_t output_pin_from_channel(OutputChannel channel) {
  switch (channel) {
    case OUTPUT_CHANNEL_1:
      return PIN_OUT_CHANNEL_1;
    case OUTPUT_CHANNEL_2:
      return PIN_OUT_CHANNEL_2;
    case OUTPUT_CHANNEL_3:
      return PIN_OUT_CHANNEL_3;
    default:
      return PIN_OUT_OFFBEAT;
  }
}

// cppcheck-suppress unusedFunction
void output_set(OutputChannel channel, bool value) {
  uint8_t pin = output_pin_from_channel(channel);
  digitalWrite(pin, (value) ? HIGH : LOW); // pulse out
}

// cppcheck-suppress unusedFunction
void output_clear_all(void) {
  output_set_low(OUTPUT_CHANNEL_1);
  output_set_low(OUTPUT_CHANNEL_2);
  output_set_low(OUTPUT_CHANNEL_3);
  output_set_low(OUTPUT_CHANNEL_OFFBEAT);
}
