#include "euclidean.h"

#include <stdbool.h>

/* INTERNAL */

/// Find the length of a binary number by counting bitwise
static uint8_t binary_count_digits(uint16_t bnry) {
  bool lengthfound = false;
  uint8_t length = 1; // no number can have a length of zero - single 0 has a length of one, but no 1s for the system to count
  for (int q = 32; q >= 0; q--) {
    uint8_t r = (bnry >> q) & 0x01;
    if (r == 1 && lengthfound == false) {
      length = q + 1;
      lengthfound = true;
    }
  }
  return length;
}

/// Concatenate two binary numbers bitwise
static uint16_t binary_concat(uint16_t a, uint16_t b) {
  uint8_t b_len = binary_count_digits(b);
  uint16_t sum = (a << b_len);
  sum = sum | b;
  return sum;
}

// Euclidean pattern generation function with regressions
static uint16_t euclidean_pattern_old(uint8_t length, uint8_t density);

/* EXTERNAL */

// cppcheck-suppress unusedFunction
uint16_t euclidean_string(uint8_t length, uint8_t density, uint8_t offset) {
  uint16_t pattern = euclidean_pattern_old(length, density);
  pattern = pattern_rotate(pattern, length, offset);
  return pattern;
}

uint16_t euclidean_pattern(uint8_t length, uint8_t density) {
  // Constraint: density does not exceed length
  density = (length < density) ? length : density;

  // Link to original paper:
  // http://cgm.cs.mcgill.ca/~godfried/publications/banff.pdf

  return 0;
}

static uint16_t euclidean_pattern_old(uint8_t length, uint8_t density) {
  // Constraint: density does not exceed length
  density = (length < density) ? length : density;

  // Link to original paper:
  // http://cgm.cs.mcgill.ca/~godfried/publications/banff.pdf

  // Stores the sequences of bits that are rearranged and concatenated to form 
  // the final Euclidean rhythm pattern.
  //
  // The length of each bit sequence is not stored, because the leftmost digit
  // is always 1, and the length is recalculated every time it is needed.
  uint16_t sequences[length];
  uint8_t sequences_len = length;
  
  // Populate sequences with sequences of length 1 unsorted pulses and pauses
  for (uint8_t a = 0; a < length; a++) { 
    bool bit = (a < density);
    sequences[a] = bit;
  }

  uint8_t pauses = length - density;
  uint8_t pauses_per_pulse = pauses / density;
  uint8_t remainder = pauses % density;
  if ((pauses_per_pulse > 0) && (remainder < 2)) { 
    // Easy case, when there is a 0 or 1 remainder

    for (uint8_t a = 0; a < density; a++) {
      for (uint8_t b = sequences_len - 1; b > sequences_len - pauses_per_pulse - 1; b--) {
        sequences[a] = binary_concat(sequences[a], sequences[b]);
      }
      sequences_len = sequences_len - pauses_per_pulse;
    }
  } else {
    if (density == 0) {
      density = 1;  //	Prevent crashes when density=0 and length goes from 0 to 1
    }
    uint8_t groupa = density;
    uint8_t groupb = pauses;

    // Main loop
    while (groupb > 1) { 
      uint8_t trim_count = 0;

      if (groupa > groupb) { 
        uint8_t a_remainder = groupa - groupb; // What will be left of groupa once group B is interleaved

        // Count through the matching sets of A, ignoring remaindered
        for (uint8_t a = 0; a < groupa - a_remainder; a++) {
          sequences[a] = binary_concat(sequences[a], sequences[sequences_len - 1 - a]);
          trim_count++;
        }
        sequences_len = sequences_len - trim_count;

        groupa = groupb;
        groupb = a_remainder;
      } else if (groupb > groupa) {
        uint8_t b_remainder = groupb - groupa; // What will be left of group once group A is interleaved

        // Count from right back through the Bs
        for (uint8_t a = sequences_len - 1; a >= groupa + b_remainder; a--) {
          sequences[sequences_len - a - 1] = binary_concat(sequences[sequences_len - a - 1], sequences[a]);

          trim_count++;
        }
        sequences_len = sequences_len - trim_count;

        groupb = b_remainder;
      } else {
        for (uint8_t a = 0; a < groupa; a++) {
          sequences[a] = binary_concat(sequences[a], sequences[sequences_len - 1 - a]);
          trim_count++;
        }
        sequences_len = sequences_len - trim_count;

        groupb = 0;
      }
    }
  }

  // Concatenate sequences into result - according to sequences_len
  uint16_t result = 0; 
  for (uint8_t a = 0; a < sequences_len; a++) {
    result = binary_concat(result, sequences[a]);
  }

  return result;
}

/// Right-rotate the pattern of the given length pattern_length by the given
/// offset, wrapping around.
///
// cppcheck-suppress unusedFunction
uint16_t pattern_rotate(uint16_t pattern, uint8_t pattern_len, uint8_t offset) {
  // Constraint: offset does not exceed pattern length
  offset = (pattern_len < offset) ? pattern_len : offset;

  if (offset == 0) { return pattern; }

  // Create a mask of all 1s that is pattern_len long
  uint16_t mask = ((1 << pattern_len) - 1);

  // Ignore any bits that are beyond pattern_len
  pattern &= mask;

  // Bits that do not get wrapped, they just get shifted right
  uint16_t pattern_shifted = pattern >> offset;

  // Bits that get wrapped around to the left
  uint16_t pattern_wrapped = pattern << (pattern_len - offset);

  // Recombine the two parts of the pattern
  uint16_t result = pattern_shifted | pattern_wrapped;

  // Ignore any bits beyond pattern_len (which could have been generated by wrapping)
  result &= mask;

  return result;
}