#ifndef EUCLIDEAN_H_
#define EUCLIDEAN_H_

#include <stdint.h>

/// Euclid calculation function
/// Returns a 16-step Euclidean rhythm in the form of a bit flag (beat flag?)
/// @param n Length: Number of beats rhythm will take.
/// @param k Density: Number of beats to be on.
/// @param o Offset: Rotation of the beat. May not exceed length.
uint16_t euclid(int n, int k, int o);

uint16_t rightRotate(int shift, uint16_t value, uint8_t pattern_length);
int findlength(unsigned int bnry);
unsigned int ConcatBin(unsigned int bina, unsigned int binb);

#endif /* EUCLIDEAN_H_ */ 