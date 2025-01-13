#include "euclidean.h"

#include <stdbool.h>

/* INTERNAL */

/// Right-rotate the pattern of the given length pattern_length by the given
/// offset, wrapping around.
static uint16_t pattern_offset(uint16_t pattern, uint8_t pattern_len, uint8_t offset) {
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

/* EXTERNAL */

// cppcheck-suppress unusedFunction
uint16_t euclid(uint8_t length, uint8_t density, uint8_t offset) {
  int pauses = length - density;
  int per_pulse = pauses / density;
  int remainder = pauses % density;

  uint16_t workbeat[length];
  int workbeat_count = length;

  uint16_t outbeat;
  uint16_t outbeat2;

  // Populate workbeat with unsorted pulses and pauses
  for (uint8_t a = 0; a < length; a++) { 
    bool bit = (a < density);
    workbeat[a] = bit;
  }

  if (per_pulse > 0 && remainder < 2) { 
    // Handle easy cases where there is no or only one remainer

    for (int a = 0; a < density; a++) {
      for (int b = workbeat_count - 1; b > workbeat_count - per_pulse - 1; b--) {
        workbeat[a] = binary_concat(workbeat[a], workbeat[b]);
      }
      workbeat_count = workbeat_count - per_pulse;
    }

    outbeat = 0; // Concatenate workbeat into outbeat - according to workbeat_count
    for (int a = 0; a < workbeat_count; a++) {
      outbeat = binary_concat(outbeat, workbeat[a]);
    }

    if (offset > 0) {
      outbeat2 = pattern_offset(outbeat, length, offset); // Add offset to the step pattern
    } else {
      outbeat2 = outbeat;
    }

    return outbeat2;
  } else {
    if (density == 0) {
      density = 1;  //	Prevent crashes when density=0 and length goes from 0 to 1
    }
    int groupa = density;
    int groupb = pauses;

    // Main recursive loop
    while (groupb > 1) { 
      int trim_count = 0;

      if (groupa > groupb) { 
        int a_remainder = groupa - groupb; // what will be left of groupa once groupB is interleaved
        for (int a = 0; a < groupa - a_remainder; a++) { //count through the matching sets of A, ignoring remaindered
          workbeat[a] = binary_concat(workbeat[a], workbeat[workbeat_count - 1 - a]);
          trim_count++;
        }
        workbeat_count = workbeat_count - trim_count;

        groupa = groupb;
        groupb = a_remainder;
      } else if (groupb > groupa) {
        int b_remainder = groupb - groupa; // what will be left of group once group A is interleaved
        for (int a = workbeat_count - 1; a >= groupa + b_remainder; a--) { //count from right back through the Bs
          workbeat[workbeat_count - a - 1] = binary_concat(workbeat[workbeat_count - a - 1], workbeat[a]);

          trim_count++;
        }
        workbeat_count = workbeat_count - trim_count;
        groupb = b_remainder;
      } else {
        for (int a = 0; a < groupa; a++) {
          workbeat[a] = binary_concat(workbeat[a], workbeat[workbeat_count - 1 - a]);
          trim_count++;
        }
        workbeat_count = workbeat_count - trim_count;
        groupb = 0;
      }
    }

    // Concatenate workbeat into outbeat - according to workbeat_count
    outbeat = 0; 
    for (int a = 0; a < workbeat_count; a++) {
      outbeat = binary_concat(outbeat, workbeat[a]);
    }

    if (offset > 0) {
      // Add offset to the step pattern
      outbeat2 = pattern_offset(outbeat, length, offset);
    } else {
      outbeat2 = outbeat;
    }

    return outbeat2;
  }
}
