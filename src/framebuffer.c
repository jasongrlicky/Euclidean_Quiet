#include "framebuffer.h"

// cppcheck-suppress unusedFunction
void framebuffer_pixel_set(uint8_t x, uint8_t y, Color color) {
  // Clear existing color
  uint16_t mask = 0x0003; // Must be 16 bits because it gets inverted
  framebuffer[y] &= ~(mask << (x * 2));

  // Set new color
  framebuffer[y] |= (color << (x * 2));
}

// cppcheck-suppress unusedFunction
void framebuffer_pixel_set_fast(uint8_t x, uint8_t y, Color color) {
  // Set new color
  framebuffer[y] |= (color << (x * 2));
}

// cppcheck-suppress unusedFunction
void framebuffer_row_set(uint8_t y, uint16_t pixels) {
  framebuffer[y] = pixels;
}