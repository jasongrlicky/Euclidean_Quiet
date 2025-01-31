#ifndef LED_SLEEP_H_
#define LED_SLEEP_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "types.h"

typedef enum LedSleepUpdate {
    LED_SLEEP_UPDATE_NONE,
    LED_SLEEP_UPDATE_WAKE,
    LED_SLEEP_UPDATE_SLEEP,
} LedSleepUpdate;

void led_sleep_reset(Milliseconds now);

LedSleepUpdate led_sleep_update(Milliseconds now);

#ifdef __cplusplus
}
#endif
#endif /* LED_SLEEP_H_ */ 