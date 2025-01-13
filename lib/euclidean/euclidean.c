#include "euclidean.h"

#include <stdbool.h>

/* INTERNAL */

/// Right-rotate the pattern of the given length pattern_length by the given
/// ofset, wrapping around.
static uint16_t pattern_offset(uint16_t pattern, uint8_t pattern_len, uint8_t offset) {
  uint16_t mask = ((1 << pattern_len) - 1);
  pattern &= mask;
  return ((pattern >> offset) | (pattern << (pattern_len - offset))) & mask;
}

/// Find the length of a binary number by counting bitwise
static int binary_count_digits(unsigned int bnry) {
  bool lengthfound = false;
  int length = 1; // no number can have a length of zero - single 0 has a length of one, but no 1s for the sytem to count
  for (int q = 32; q >= 0; q--) {
    int r = (bnry >> q) & 0x01;
    if (r == 1 && lengthfound == false) {
      length = q + 1;
      lengthfound = true;
    }
  }
  return length;
}

/// Concatenate two binary numbers bitwise
static unsigned int binary_concat(unsigned int bina, unsigned int binb) {
  int binb_len = binary_count_digits(binb);
  unsigned int sum = (bina << binb_len);
  sum = sum | binb;
  return sum;
}

/* EXTERNAL */

// cppcheck-suppress unusedFunction
uint16_t euclid(int n, int k, int o) { // inputs: n=total, k=beats, o = offset
  int pauses = n - k;
  int pulses = k;
  int offset = o;
  int steps = n;
  int per_pulse = pauses / k;
  int remainder = pauses % pulses;
  unsigned int workbeat[n];
  unsigned int outbeat;
  uint16_t outbeat2;
  int workbeat_count = n;

  for (int a = 0; a < n; a++) { // Populate workbeat with unsorted pulses and pauses
    if (a < pulses) {
      workbeat[a] = 1;
    } else {
      workbeat[a] = 0;
    }
  }

  if (per_pulse > 0 && remainder < 2) { // Handle easy cases where there is no or only one remainer
    for (int a = 0; a < pulses; a++) {
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
      outbeat2 = pattern_offset(outbeat, steps, offset); // Add offset to the step pattern
    } else {
      outbeat2 = outbeat;
    }

    return outbeat2;
  } else {
    if (pulses == 0) {
      pulses = 1;  //	Prevent crashes when k=0 and n goes from 0 to 1
    }
    int groupa = pulses;
    int groupb = pauses;

    while (groupb > 1) { //main recursive loop
      int trim_count = 0;

      if (groupa > groupb) { // more Group A than Group B
        int a_remainder = groupa - groupb; // what will be left of groupa once groupB is interleaved
        for (int a = 0; a < groupa - a_remainder; a++) { //count through the matching sets of A, ignoring remaindered
          workbeat[a] = binary_concat(workbeat[a], workbeat[workbeat_count - 1 - a]);
          trim_count++;
        }
        workbeat_count = workbeat_count - trim_count;

        groupa = groupb;
        groupb = a_remainder;
      } else if (groupb > groupa) { // More Group B than Group A
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

    outbeat = 0; // Concatenate workbeat into outbeat - according to workbeat_count
    for (int a = 0; a < workbeat_count; a++) {
      outbeat = binary_concat(outbeat, workbeat[a]);
    }

    if (offset > 0) {
      outbeat2 = pattern_offset(outbeat, steps, offset); // Add offset to the step pattern
    } else {
      outbeat2 = outbeat;
    }

    return outbeat2;
  }
}
