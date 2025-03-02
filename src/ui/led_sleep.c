#include "led_sleep.h"

#include "common/timeout.h"
#include "config.h"
#include "hardware/led.h"

/* DATA STRUCTURES */

typedef enum LedSleepState {
	LED_SLEEP_STATE_WAKE,
	LED_SLEEP_STATE_DIM,
	LED_SLEEP_STATE_SLEEP,
} LedSleepState;

typedef enum LedSleepUpdate {
	LED_SLEEP_UPDATE_NONE,
	LED_SLEEP_UPDATE_WAKE,
	LED_SLEEP_UPDATE_DIM,
	LED_SLEEP_UPDATE_SLEEP,
} LedSleepUpdate;

/* GLOBALS */

static LedSleepState state = LED_SLEEP_STATE_WAKE;

static Timeout dim_timeout = {.duration = LED_DIM_TIME};
static Timeout sleep_timeout = {.duration = LED_SLEEP_TIME};

/* DECLARATION */

static LedSleepUpdate led_sleep_decide(bool postpone_sleep, Milliseconds now);

/* EXTERNAL */

// cppcheck-suppress unusedFunction
void led_sleep_init(Milliseconds now) {
	timeout_reset(&dim_timeout, now);
	timeout_reset(&sleep_timeout, now);
}

// cppcheck-suppress unusedFunction
void led_sleep_update(bool postpone_sleep, Milliseconds now) {
	const LedSleepUpdate sleep_update = led_sleep_decide(postpone_sleep, now);
	if (sleep_update == LED_SLEEP_UPDATE_WAKE) {
		led_wake();
	} else if (sleep_update == LED_SLEEP_UPDATE_DIM) {
		led_dim();
	} else if (sleep_update == LED_SLEEP_UPDATE_SLEEP) {
		led_sleep();
	}
}

/* INTERNAL */

static LedSleepUpdate led_sleep_decide(bool postpone_sleep, Milliseconds now) {
	// Handle transition to wake state
	if (postpone_sleep) {
		timeout_reset(&dim_timeout, now);
		timeout_reset(&sleep_timeout, now);

		LedSleepUpdate update = LED_SLEEP_UPDATE_NONE;
		if (state != LED_SLEEP_STATE_WAKE) {
			state = LED_SLEEP_STATE_WAKE;
			update = LED_SLEEP_UPDATE_WAKE;
		}

		return update;
	}

	// Handle transition from wake to dim
	if (state == LED_SLEEP_STATE_WAKE) {
		if (timeout_fired(&dim_timeout, now)) {
			state = LED_SLEEP_STATE_DIM;
			return LED_SLEEP_UPDATE_DIM;
		}
	}

	// Handle transition from dim to sleep
	if (state == LED_SLEEP_STATE_DIM) {
		if (timeout_fired(&sleep_timeout, now)) {
			state = LED_SLEEP_STATE_SLEEP;
			return LED_SLEEP_UPDATE_SLEEP;
		}
	}

	return LED_SLEEP_UPDATE_NONE;
}
