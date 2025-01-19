#ifndef TIMEOUT_H_
#define TIMEOUT_H_

#include <stdbool.h>

#include "types.h"

/// Manually checked simple non-repeating timeout that operates in milliseconds
typedef struct Timeout {
  /// How long after `start` the timeout will be considered fired
  Milliseconds duration;
  /// When the timeout started
  Milliseconds start;
} Timeout;

/// Make the timeout start again at `now`.
void timeout_reset(Timeout *timeout, Milliseconds now);

/// Check if the timeout has fired, given the current time, `now`
bool timeout_fired(Timeout const *timeout, Milliseconds now);

#endif /* TIMEOUT_H_ */ 
