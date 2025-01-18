#ifndef CONFIG_H_
#define CONFIG_H_

#define LED_BRIGHTNESS 5 // From 0 (low) to 15
#define ADJUSTMENT_DISPLAY_TIME 450 // How long adjustments (such as to channel pattern length) are displayed for
#define LED_SLEEP_TIME 300000 // Milliseconds until LED matrix sleeps (5 minutes)
#define READ_DELAY 50 // For debouncing
#define INTERNAL_CLOCK_PERIOD 125 // Milliseconds between internal clock ticks

/* FEATURES */

#define INTERNAL_CLOCK_DEFAULT 0 // 0 = Internal clock begins disabled, 1 = begins enabled
#define EEPROM_READ 1 // 0 = Reading from EEPROM disabled, 1 = enabled
#define EEPROM_WRITE 1 // 0 = Writing to EEPROM disabled, 1 = enabled
#define LOGGING_ENABLED 0 // 0 = Logging over serial disabled, 1 = enabled
#define LOGGING_PERIODIC_ENABLED 0 // 0 = Logging every LOGGING_INTERVAL disabled, 1 = enabled
#define LOGGING_INTERVAL 1000 // Milliseconds between periodic log messages

#endif /* CONFIG_H_ */ 
