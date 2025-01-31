#ifndef LED_H_
#define LED_H_

#include <LedControl.h>

#include "hardware.h"

/// Row of LED Display marked "CH SEL" on panel
#define LED_CH_SEL_Y 6
/// Column of LED Display marked "TRIG" on panel
#define LED_IN_TRIG_X 0
/// Column of LED Display where Reset input is indicated
#define LED_IN_RESET_X 1
/// Column of LED Display marked "1" on panel
#define LED_OUT_CH1_X 2
/// Column of LED Display marked "OFF" on panel
#define LED_OUT_OFFBEAT_X 3
/// Column of LED Display marked "2" on panel
#define LED_OUT_CH2_X 5
/// Column of LED Display marked "3" on panel
#define LED_OUT_CH3_X 7
/// Row of LED Display marked "OUT" on panel
#define LED_OUT_Y 7

void led_init(void);
void led_set_row(uint8_t row, uint8_t pixels);
void led_sleep();
void led_wake();

#endif /* LED_H_ */ 
