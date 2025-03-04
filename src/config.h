#ifndef CONFIG_H_
#define CONFIG_H_
#ifdef __cplusplus
extern "C" {
#endif

// clang-format off

#define LED_BRIGHTNESS 5 // From 0 (low) to 15
#define LED_BRIGHTNESS_DIM 1 // From 0 (low) to 15
#define ADJUSTMENT_DISPLAY_TIME 450 // How long adjustments (such as to channel pattern length) are displayed for
#define LED_DIM_TIME 240000 // Milliseconds until LED matrix dims (4 minutes)
#define LED_SLEEP_TIME 300000 // Milliseconds until LED matrix sleeps (5 minutes)
#define INPUT_INDICATOR_FLASH_TIME 16 // Milliseconds input indicators flash when being illuminated
#define PLAYHEAD_FLASH_TIME_DEFAULT 180 // Milliseconds playhead is visible for when it changes
#define PLAYHEAD_IDLE_TIME 2000 // Milliseconds after moving that playhead begins to flash automatically
#define PLAYHEAD_IDLE_LOOP_PERIOD 3500 // Milliseconds for period of playhead idle flashing loop
#define ANIM_BLINK_INTERVAL 32 // Default animation frame interval for the `COLOR_BLINK` palette color, in milliseconds
#define ANIM_ANTS_INTERVAL 24 // Default animation frame interval for the `COLOR_ANTS` palette color, in milliseconds
#define READ_DELAY 50 // For debouncing
#define INTERNAL_CLOCK_PERIOD 125 // Milliseconds between internal clock ticks

/* FEATURES */

#define INTERNAL_CLOCK_DEFAULT 0 // 0 = Internal clock begins disabled, 1 = begins enabled
#define EEPROM_READ 1 // 0 = Reading from EEPROM disabled, 1 = enabled
#define EEPROM_WRITE 1 // 0 = Writing to EEPROM disabled, 1 = enabled

/* DEBUG FEATURES */

#define LOGGING_ENABLED 1 // 0 = Logging over serial disabled, 1 = enabled
#define LOGGING_INPUT 0 // 0 = Don't log Input events, 1 = Log input events
#define LOGGING_EEPROM 0 // 0 = Don't log EEPROM writes, 1 = Log EEPROM writes
#define LOGGING_CYCLE_TIME 1 // 0 = Don't log max cycle time in the last interval, 1 = Do log max cycle time
#define LOGGING_CYCLE_TIME_INTERVAL 1000 // Milliseconds to capture the max cycle time during

// clang-format on

#ifdef __cplusplus
}
#endif
#endif /* CONFIG_H_ */
