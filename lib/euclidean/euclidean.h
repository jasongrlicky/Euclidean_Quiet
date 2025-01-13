#ifndef EUCLIDEAN_H_
#define EUCLIDEAN_H_

#include <stdint.h>

/// Convenience function for generating a Euclidean rhythm and rotating it at 
/// the same time.
///
/// Generates a Euclidean rhythm pattern up to 16 steps in length, with an offset.
/// @param length Number of total steps in the pattern.
/// @param density Number of active steps in the pattern.
/// @param offset: Rotation of the pattern to the right.
/// @returns Pattern in the form of a bit flag (beat flag?)
uint16_t euclidean_rhythm_gen(uint8_t length, uint8_t density, uint8_t offset);

/// Euclid calculation function
/// Generates a Euclidean rhythm pattern up to 16 steps in length.
/// @param length Number of total steps in the pattern.
/// @param density Number of active steps in the pattern.
/// @returns Pattern in the form of a bit flag (beat flag?), stored in the 
/// lowest-order bits of the return value.
uint16_t euclid(uint8_t length, uint8_t density);

/// Right-rotate the `pattern` of the given length `pattern_len` by the given
/// offset, wrapping around.
/// @param pattern The steps for the pattern, represented as 16 bit flags stored
/// in the lowest-order bits.
/// @param pattern_len Number of total steps in the pattern.
/// @param offset Number of steps to rotate `pattern` right by
/// @returns Pattern in the form of a bit flag (beat flag?), stored in the 
/// lowest-order bits of the return value.
uint16_t pattern_offset(uint16_t pattern, uint8_t pattern_len, uint8_t offset);

#endif /* EUCLIDEAN_H_ */ 