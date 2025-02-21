#ifndef LED_SLEEP_H_
#define LED_SLEEP_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "common/types.h"

void led_sleep_init(Milliseconds now);
void led_sleep_update(bool postpone_sleep, Milliseconds now);

#ifdef __cplusplus
}
#endif
#endif /* LED_SLEEP_H_ */