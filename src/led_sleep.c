#include "led_sleep.h"

#include "config.h"
#include "timeout.h"

static bool led_sleep_mode_active = false;

static Timeout led_sleep_timeout = { .duration = LED_SLEEP_TIME };

void led_sleep_reset(Milliseconds now) {
    timeout_reset(&led_sleep_timeout, now);
}

LedSleepUpdate led_sleep_update(Milliseconds now) {
  LedSleepUpdate update = LED_SLEEP_UPDATE_NONE;
  if (led_sleep_mode_active) {
    // LED is sleeping:
    // If it has been less than LED_SLEEP_TIME since an interaction event has
    // been received, wake the LED
    if (!timeout_fired(&led_sleep_timeout, now)) {
        led_sleep_mode_active = false;
        update = LED_SLEEP_UPDATE_WAKE;
    }
  } else {
    // LED is awake:
    // Sleep it if no inputs have been received or generated since LED_SLEEP_TIME ago
    if (timeout_fired(&led_sleep_timeout, now)) {
        led_sleep_mode_active = true;
        update = LED_SLEEP_UPDATE_SLEEP;
    }
  }
  return update;
}
