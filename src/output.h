#ifndef OUTPUT_H_
#define OUTPUT_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

// Indices for individual output channels
typedef enum OutputChannel {
	OUTPUT_CHANNEL_1 = 0,
	OUTPUT_CHANNEL_2 = 1,
	OUTPUT_CHANNEL_3 = 2,
	OUTPUT_CHANNEL_OFFBEAT = 3
} OutputChannel;

#define OUTPUT_NUM_CHANNELS 4

#define output_set_high(channel) (output_set(channel, true))
#define output_set_low(channel) (output_set(channel, false))
/// Send output signals on the actual hardware
void output_set(OutputChannel channel, bool value);

void output_clear_all(void);

#ifdef __cplusplus
}
#endif
#endif /* OUTPUT_H_ */