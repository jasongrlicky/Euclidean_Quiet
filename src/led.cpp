#include "led.h"

#include "config.h"

bool led_sleep_mode_active = false;

// Initialize objects for controlling LED matrix
// (from LedControl.h library)
// 1 is maximum number of devices that can be controlled
LedControl lc = LedControl(PIN_OUT_LED_DATA, PIN_OUT_LED_CLOCK, PIN_OUT_LED_SELECT, 1);

/* DECLARATIONS */

static void led_anim_wake();
static void led_anim_sleep();

/* EXTERNAL */

/// Initialize the MAX72XX LED Matrix
void led_init(void) {
  // The LED matrix is in power-saving mode on startup.
  // Set power-saving mode to false to wake it up
  lc.shutdown(LED_ADDR, false);
  lc.setIntensity(LED_ADDR, LED_BRIGHTNESS);
  lc.clearDisplay(LED_ADDR);
}
void led_sleep() {
  led_sleep_mode_active = true;

  led_anim_sleep();
  lc.shutdown(LED_ADDR, true);
}

void led_wake() {
  led_sleep_mode_active = false;

  lc.shutdown(LED_ADDR, false);
  led_anim_wake();
}

/* INTERNAL */

static void led_anim_wake() { 
  for (uint8_t step = 0; step < 4; step++) {
    uint8_t a = 3 - step;
    lc.setRow(LED_ADDR, a, 255);
    lc.setRow(LED_ADDR, 7 - a, 255);
    delay(100);
    lc.setRow(LED_ADDR, a, 0);
    lc.setRow(LED_ADDR, 7 - a, 0);
  }
}

static void led_anim_sleep() {
  for (uint8_t a = 0; a < 4; a++) {
    lc.setRow(LED_ADDR, a, 255);
    lc.setRow(LED_ADDR, 7 - a, 255);
    delay(200);
    lc.setRow(LED_ADDR, a, 0);
    lc.setRow(LED_ADDR, 7 - a, 0);
  }
}
