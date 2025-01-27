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

/// Check if the timeout has fired, given the current time, `now`. If it has 
/// fired, resets the timeout, so it becomes periodic.
bool timeout_fired_loop(Timeout *timeout, Milliseconds now);


/// Similar to `Timeout`, but only fires the first time after it fires, until it
/// has been reset.
typedef struct TimeoutOnce {
  Timeout inner;
  /// Is `true` until the timeout has fired once, then is `false` until the 
  /// timeout has been reset.
  bool active;
} TimeoutOnce;

/// Make the timeout start again at `now`.
void timeout_once_reset(TimeoutOnce *timeout_once, Milliseconds now);

/// Check if the timeout has fired, given the current time, `now`. Only returns
/// `true` if this is the first time after the timer has fired.
bool timeout_once_fired(TimeoutOnce *timeout_once, Milliseconds now);

#endif /* TIMEOUT_H_ */ 
