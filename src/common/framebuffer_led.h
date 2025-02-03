#ifndef FRAMEBUFFER_LED_H_
#define FRAMEBUFFER_LED_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "common/types.h"

/// Update the animations for framebuffer colors
void framebuffer_update_color_animations(Milliseconds now);

/// Copy one row of the framebuffer to the LED matrix. We only copy one row of
/// the framebuffer to the LED matrix per cycle to avoid having to wait on the
/// display driver chip.
void framebuffer_copy_row_to_display(void);

#ifdef __cplusplus
}
#endif
#endif /* FRAMEBUFFER_LED_H_ */
