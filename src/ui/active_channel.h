#ifndef ACTIVE_CHANNEL_H_
#define ACTIVE_CHANNEL_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "common/types.h"
#include "framebuffer.h"

void active_channel_display_draw(Framebuffer *fb, Channel active_channel);

#ifdef __cplusplus
}
#endif
#endif /* ACTIVE_CHANNEL_H_ */
