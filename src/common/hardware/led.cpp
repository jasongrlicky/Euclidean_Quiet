#include "led.h"

#include "common/hardware.h"
#include "common/hardware/pins.h"
#include "config.h"

#include <LedControl.h>

// Initialize objects for controlling LED matrix
// (from LedControl.h library)
// 1 is maximum number of devices that can be controlled
static LedControl lc = LedControl(PIN_OUT_LED_DATA, PIN_OUT_LED_CLOCK, PIN_OUT_LED_SELECT, 1);

/* EXTERNAL */

// cppcheck-suppress unusedFunction
void led_init(void) {
	// The LED matrix is in power-saving mode on startup.
	// Set power-saving mode to false to wake it up
	lc.shutdown(LED_ADDR, false);
	lc.setIntensity(LED_ADDR, LED_BRIGHTNESS);
	lc.clearDisplay(LED_ADDR);
}

// cppcheck-suppress unusedFunction
void led_set_row(uint8_t row, uint8_t pixels) { lc.setRow(LED_ADDR, row, pixels); }

// cppcheck-suppress unusedFunction
void led_sleep() { lc.shutdown(LED_ADDR, true); }

// cppcheck-suppress unusedFunction
void led_dim() { lc.setIntensity(LED_ADDR, LED_BRIGHTNESS_DIM); }

// cppcheck-suppress unusedFunction
void led_wake() {
	lc.shutdown(LED_ADDR, false);
	lc.setIntensity(LED_ADDR, LED_BRIGHTNESS);
}
