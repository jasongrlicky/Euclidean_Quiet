#ifndef EUCLIDEAN_H_
#define EUCLIDEAN_H_

#include <stdint.h>

/// Euclid calculation function
/// Generates a Euclidean rhythm pattern up to 16 steps in length.
/// @param length Number of total steps in the pattern.
/// @param density Number of active steps in the pattern.
/// @param offset: Rotation of the pattern to the right.
/// @returns Pattern in the form of a bit flag (beat flag?)
uint16_t euclidean_rhythm_gen(uint8_t length, uint8_t density, uint8_t offset);

#endif /* EUCLIDEAN_H_ */ 