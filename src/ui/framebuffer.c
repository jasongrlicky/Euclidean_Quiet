#include "framebuffer.h"

// cppcheck-suppress unusedFunction
void framebuffer_pixel_set(Framebuffer *fb, uint8_t x, uint8_t y, Color color) {
	// Clear existing color
	const uint16_t mask = 0x0003; // Must be 16 bits because it gets inverted
	fb->data[y] &= ~(mask << (x * 2));

	// Set new color
	fb->data[y] |= (color << (x * 2));
}

// cppcheck-suppress unusedFunction
void framebuffer_pixel_set_fast(Framebuffer *fb, uint8_t x, uint8_t y, Color color) {
	// Set new color
	fb->data[y] |= (color << (x * 2));
}

// cppcheck-suppress unusedFunction
void framebuffer_pixel_on(Framebuffer *fb, uint8_t x, uint8_t y) {
	framebuffer_pixel_set(fb, x, y, COLOR_ON);
}

// cppcheck-suppress unusedFunction
void framebuffer_pixel_on_fast(Framebuffer *fb, uint8_t x, uint8_t y) {
	framebuffer_pixel_set_fast(fb, x, y, COLOR_ON);
}

// cppcheck-suppress unusedFunction
void framebuffer_pixel_off(Framebuffer *fb, uint8_t x, uint8_t y) {
	framebuffer_pixel_set(fb, x, y, COLOR_OFF);
}

// cppcheck-suppress unusedFunction
void framebuffer_pixel_off_fast(Framebuffer *fb, uint8_t x, uint8_t y) {
	framebuffer_pixel_set_fast(fb, x, y, COLOR_OFF);
}

// cppcheck-suppress unusedFunction
void framebuffer_row_off(Framebuffer *fb, uint8_t y) { framebuffer_row_set(fb, y, 0); }

// cppcheck-suppress unusedFunction
void framebuffer_row_set(Framebuffer *fb, uint8_t y, uint16_t pixels) { fb->data[y] = pixels; }