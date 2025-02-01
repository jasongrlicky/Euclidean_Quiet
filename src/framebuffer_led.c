#include "framebuffer_led.h"

#include "config.h"
#include "framebuffer.h"
#include "led.h"
#include "timeout.h"

extern uint16_t framebuffer[LED_ROWS];

/// To keep latency from spiking, we only draw one row of the framebuffer to the
/// LED matrix at a time. The row that gets drawn rotates between the 8 rows of 
/// the framebuffer to keep visual latency equal for all rows. 
static uint8_t framebuffer_out_row;

#define ANIM_DAZZLE_NUM_FRAMES 2
static Timeout anim_dazzle_timeout = { .duration = ANIM_DAZZLE_INTERVAL };
static uint8_t anim_dazzle_frame = 0;

#define ANIM_ANTS_NUM_FRAMES 4
static Timeout anim_ants_timeout = { .duration = ANIM_ANTS_INTERVAL };
static uint8_t anim_ants_frame = 0;

/* DECLARATIONS */

static inline uint8_t anim_dazzle(uint8_t frame, uint8_t x, uint8_t y);
static inline uint8_t anim_marching_ants(uint8_t frame, uint8_t x, uint8_t y);

/* EXTERNAL */

// cppcheck-suppress unusedFunction
void framebuffer_copy_row_to_display() {
  uint8_t row = (framebuffer_out_row) % LED_ROWS;
  uint16_t fb_row_bits = framebuffer[row];

  uint8_t to_draw = 0;
  for (uint8_t col = 0; col < LED_COLUMNS; col++) {
    Color color = (Color)((fb_row_bits >> (col * 2)) & 0b00000011);

    if (color == COLOR_ANTS) {
      to_draw |= (anim_marching_ants(anim_ants_frame, col, row) << col);
    } else if (color == COLOR_DAZZLE) {
      to_draw |= (anim_dazzle(anim_dazzle_frame, col, row) << col);
    } else {
      to_draw |= (color << col);
    }
  }

  led_set_row(row, to_draw);

  // Next cycle, copy the next row of the framebuffer to the LED matrix
  framebuffer_out_row = (framebuffer_out_row + 1) % LED_ROWS;
}

// cppcheck-suppress unusedFunction
void framebuffer_update_color_animations(Milliseconds now) {
  if(timeout_loop(&anim_dazzle_timeout, now)) {
    anim_dazzle_frame = (anim_dazzle_frame + 1) % ANIM_DAZZLE_NUM_FRAMES;
  }

  if(timeout_loop(&anim_ants_timeout, now)) {
    anim_ants_frame = (anim_ants_frame + 1) % ANIM_ANTS_NUM_FRAMES;
  }
}

/* INTERNAL */

static inline uint8_t anim_dazzle(uint8_t frame, uint8_t x, uint8_t y) {
  return ((x + y + frame) % ANIM_DAZZLE_NUM_FRAMES);
}

static inline uint8_t anim_marching_ants(uint8_t frame, uint8_t x, uint8_t y) {
  uint8_t val = (x + y + (ANIM_ANTS_NUM_FRAMES - frame)) / 2;
  return (val % 2);
}