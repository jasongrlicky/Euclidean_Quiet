#include "led_sleep.h"

#include "config.h"
#include "timeout.h"

typedef enum LedSleepState {
    LED_SLEEP_STATE_WAKE,
    LED_SLEEP_STATE_DIM,
    LED_SLEEP_STATE_SLEEP,
} LedSleepState;

static LedSleepState led_sleep_state = LED_SLEEP_STATE_WAKE;

static Timeout led_dim_timeout = { .duration = LED_DIM_TIME };
static Timeout led_sleep_timeout = { .duration = LED_SLEEP_TIME };

void led_sleep_init(Milliseconds now) {
  timeout_reset(&led_dim_timeout, now);
  timeout_reset(&led_sleep_timeout, now);
}

LedSleepUpdate led_sleep_update(bool postpone_sleep, Milliseconds now) {
  // Handle transition to wake state
  if (postpone_sleep) {
    timeout_reset(&led_dim_timeout, now);
    timeout_reset(&led_sleep_timeout, now);

    LedSleepUpdate update = LED_SLEEP_UPDATE_NONE;
    if (led_sleep_state != LED_SLEEP_STATE_WAKE) {
      led_sleep_state = LED_SLEEP_STATE_WAKE;
      update = LED_SLEEP_UPDATE_WAKE;
    }

    return update;
  }

  // Handle transition from wake to dim
  if (led_sleep_state == LED_SLEEP_STATE_WAKE) {
    if (timeout_fired(&led_dim_timeout, now)) {
        led_sleep_state = LED_SLEEP_STATE_DIM;
        return LED_SLEEP_UPDATE_DIM;
    }
  }

  // Handle transition from dim to sleep
  if (led_sleep_state == LED_SLEEP_STATE_DIM) {
    if (timeout_fired(&led_sleep_timeout, now)) {
        led_sleep_state = LED_SLEEP_STATE_SLEEP;
        return LED_SLEEP_UPDATE_SLEEP;
    }
  }

  return LED_SLEEP_UPDATE_NONE;
}
