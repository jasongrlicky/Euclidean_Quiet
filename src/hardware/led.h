#ifndef LED_H_
#define LED_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/// Initialize the MAX72XX LED Matrix
void led_init(void);
void led_set_row(uint8_t row, uint8_t pixels);
void led_sleep();
void led_dim();
void led_wake();

#ifdef __cplusplus
}
#endif
#endif /* LED_H_ */
