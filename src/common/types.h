#ifndef TYPES_H_
#define TYPES_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

typedef unsigned long Milliseconds;
typedef unsigned long Microseconds;

/// Index into parameter tables.
typedef uint8_t ParamIdx;

/// References one of the three channels
typedef enum Channel {
	CHANNEL_1,
	CHANNEL_2,
	CHANNEL_3,
} Channel;

/// Channel that is wrapped as an optional value
typedef struct ChannelOpt {
	Channel inner;
	bool valid;
} ChannelOpt;

static const ChannelOpt CHANNEL_OPT_NONE = {.inner = CHANNEL_1, .valid = false};

#ifdef __cplusplus
}
#endif
#endif /* TYPES_H_ */