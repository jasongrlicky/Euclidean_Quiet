#include "clock.h"

#include "common/timeout.h"
#include "config.h"

static Timeout internal_clock_timeout = {.duration = INTERNAL_CLOCK_PERIOD};

static bool internal_clock_enabled = INTERNAL_CLOCK_DEFAULT;

// cppcheck-suppress unusedFunction
void internal_clock_update(InputEvents *events, Milliseconds now) {
	// Turn off internal clock when external clock received
	if (events->trig) {
		internal_clock_enabled = false;
	}

	if (events->reset) {
		timeout_reset(&internal_clock_timeout, now);
	}

	if (internal_clock_enabled && (timeout_loop(&internal_clock_timeout, now))) {
		events->internal_clock_tick = true;
	}
}