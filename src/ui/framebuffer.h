#ifndef FRAMEBUFFER_H_
#define FRAMEBUFFER_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "hardware/properties.h"

/// Represents a method of deciding on illumination of a pixel on the LED matrix
/// display.
typedef enum Color {
	/// Do not light up this pixel
	COLOR_OFF = 0,
	/// Light up this pixel
	COLOR_ON = 1,
	/// Blink the LED rapidly for this pixel
	COLOR_BLINK = 2,
	/// Show a marching ants pattern for this pixel
	COLOR_ANTS = 3
} Color;

/// Buffer that can be drawn into and manipulated before being drawn to the
/// hardware display. 2 bits per pixel, so it supports 4 colors.
typedef struct Framebuffer {
	/// Each row is stored as bits with 2 bits pixel, with 8 pixels per row
	/// indexed according to their x position. The rows are indexed according to
	/// their y position, from top to bottom, on the display.
	uint16_t data[LED_ROWS];
} Framebuffer;

Framebuffer framebuffer;

void framebuffer_pixel_on(uint8_t x, uint8_t y);
void framebuffer_pixel_on_fast(uint8_t x, uint8_t y);
void framebuffer_pixel_off(uint8_t x, uint8_t y);
void framebuffer_pixel_off_fast(uint8_t x, uint8_t y);

/// Set a single pixel on the framebuffer to the 2-bit color, using a coordinate
/// system that is not mirrored left-to-right. Overwrites existing color.
/// @param x Zero-indexed position, from left to right.
/// @param y Zero-indexed position, from top to bottom.
/// @param color 2-bit color. `COLOR_OFF` or `0` turns off pixel, `COLOR_ON`
/// or `1` turns it on.
void framebuffer_pixel_set(uint8_t x, uint8_t y, Color color);

/// Like `framebuffer_pixel_set()`, but does not overwrite the existing color - it
/// is assumed to be `COLOR_OFF`. It also does not mark the row as needing a
/// redraw. You can mark the row as needing a redraw by calling
/// `framebuffer_row_off()` before calling this function.
/// @param x Zero-indexed position, from left to right.
/// @param y Zero-indexed position, from top to bottom.
/// @param color 2-bit color. `COLOR_OFF` or `0` turns off pixel, `COLOR_ON`
/// or `1` turns it on.
void framebuffer_pixel_set_fast(uint8_t x, uint8_t y, Color color);

/// Clear a row of pixels on the framebuffer
/// @param y Zero-indexed position, from top to bottom.
void framebuffer_row_off(uint8_t y);

/// Set the color values directly for a row of pixels on the LED Matrix.
/// Colors are 2-bit.
/// @param y Zero-indexed position, from top to bottom.
void framebuffer_row_set(uint8_t y, uint16_t pixels);

#ifdef __cplusplus
}
#endif
#endif /* FRAMEBUFFER_H_ */
