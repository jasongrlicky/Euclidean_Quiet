#ifndef CONFIG_H_
#define CONFIG_H_

#define LED_BRIGHTNESS 5 // From 0 (low) to 15
#define ADJUSTMENT_DISPLAY_TIME 450 // How long adjustments (such as to channel pattern length) are displayed for
#define LED_SLEEP_TIME 300000 // Milliseconds until LED matrix sleeps (5 minutes)
#define INPUT_INDICATOR_FLASH_TIME 16 // Milliseconds input indicators flash when being illuminated
#define OUTPUT_INDICATOR_BLINK_TIME 16 // Milliseconds output indicators blink for when being illuminated
#define PALETTE_BLINK_INTERVAL 150 // Default period of the `COLOR_BLINK` palette color, in milliseconds
#define PALETTE_DAZZLE_INTERVAL 32 // Default period of the `COLOR_DAZZLE` palette color, in milliseconds
#define READ_DELAY 50 // For debouncing
#define INTERNAL_CLOCK_PERIOD 125 // Milliseconds between internal clock ticks

/* FEATURES */

#define INTERNAL_CLOCK_DEFAULT 0 // 0 = Internal clock begins disabled, 1 = begins enabled
#define EEPROM_READ 1 // 0 = Reading from EEPROM disabled, 1 = enabled
#define EEPROM_WRITE 1 // 0 = Writing to EEPROM disabled, 1 = enabled
#define FRAMEBUFFER_ENABLED 1 // 0 = Framebuffer disabled, 1 = write directly to LED

/* DEBUG FEATURES */

#define LOGGING_ENABLED 0 // 0 = Logging over serial disabled, 1 = enabled
#define LOGGING_INPUT 0 // 0 = Don't log Input events, 1 = Log input events
#define LOGGING_CH1_POSITION 0 // 0 = Don't log Channel 1 position updates, 1 = Log position updates
#define LOGGING_CYCLE_TIME 1 // 0 = Don't log max cycle time in the last interval, 1 = Do log max cycle time
#define LOGGING_CYCLE_TIME_INTERVAL 1000 // Milliseconds to capture the max cycle time during

#endif /* CONFIG_H_ */ 
