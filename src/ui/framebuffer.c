#include "framebuffer.h"

// cppcheck-suppress unusedFunction
void framebuffer_pixel_set(uint8_t x, uint8_t y, Color color) {
	// Clear existing color
	const uint16_t mask = 0x0003; // Must be 16 bits because it gets inverted
	framebuffer.data[y] &= ~(mask << (x * 2));

	// Set new color
	framebuffer.data[y] |= (color << (x * 2));
}

// cppcheck-suppress unusedFunction
void framebuffer_pixel_set_fast(uint8_t x, uint8_t y, Color color) {
	// Set new color
	framebuffer.data[y] |= (color << (x * 2));
}

// cppcheck-suppress unusedFunction
void framebuffer_pixel_on(uint8_t x, uint8_t y) { framebuffer_pixel_set(x, y, COLOR_ON); }

// cppcheck-suppress unusedFunction
void framebuffer_pixel_on_fast(uint8_t x, uint8_t y) { framebuffer_pixel_set_fast(x, y, COLOR_ON); }

// cppcheck-suppress unusedFunction
void framebuffer_pixel_off(uint8_t x, uint8_t y) { framebuffer_pixel_set(x, y, COLOR_OFF); }

// cppcheck-suppress unusedFunction
void framebuffer_pixel_off_fast(uint8_t x, uint8_t y) { framebuffer_pixel_set_fast(x, y, COLOR_OFF); }

// cppcheck-suppress unusedFunction
void framebuffer_row_off(uint8_t y) { framebuffer_row_set(y, 0); }

// cppcheck-suppress unusedFunction
void framebuffer_row_set(uint8_t y, uint16_t pixels) { framebuffer.data[y] = pixels; }