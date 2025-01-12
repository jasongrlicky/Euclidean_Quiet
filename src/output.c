#include "output.h"

#include "hardware.h"

// Bit flags: Uses 4 least significant bits to store ongoing output pulses.
// The bits are indexed according to their corresponding Channel enum member.
//
// Don't interact with this directly, use output_*() functions instead.
static uint8_t active_output_pulse_flags;

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
  // Send actual output
  uint8_t pin = output_pin_from_channel(channel);
  digitalWrite(pin, (value) ? HIGH : LOW); // pulse out

  // Note if this output is active or not
  bitWrite(active_output_pulse_flags, channel, value);
}

// cppcheck-suppress unusedFunction
void output_clear_all(void) {
  output_set_low(OUTPUT_CHANNEL_1);
  output_set_low(OUTPUT_CHANNEL_2);
  output_set_low(OUTPUT_CHANNEL_3);
  output_set_low(OUTPUT_CHANNEL_OFFBEAT);
}

// cppcheck-suppress unusedFunction
bool output_any_active(void) {
    return active_output_pulse_flags > 0;
}
