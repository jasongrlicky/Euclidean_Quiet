#ifndef CONFIG_H_
#define CONFIG_H_

#define LOGGING_ENABLED 0 // 0 = logging over serial disabled, 1 = enabled
#define LOGGING_INTERVAL 1000 // Milliseconds between periodic log messages
#define LED_BRIGHTNESS 5 // From 0 (low) to 15
#define ACTIVE_CHANNEL_DISPLAY_TIME 450 // How long active channel display is shown, in ms
#define LED_SLEEP_TIME 300000 // Milliseconds until LED matrix sleeps (5 minutes)
#define READ_DELAY 50 // For debouncing

#endif /* CONFIG_H_ */ 
