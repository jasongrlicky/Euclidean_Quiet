#include "timeout.h"

void timeout_reset(Timeout *timeout, Milliseconds now) {
  timeout->start = now;
}

bool timeout_fired(Timeout const *timeout, Milliseconds now) {
  return ((now - timeout->start) >= timeout->duration);
}

void timeout_once_reset(TimeoutOnce *timeout_once, Milliseconds now) {
  timeout_reset(&timeout_once->inner, now);
  timeout_once->active = true;
}

bool timeout_once_fired(TimeoutOnce *timeout_once, Milliseconds now) {
  if (timeout_once->active) {
    bool val = timeout_fired(&timeout_once->inner, now);
    if (val) {
      timeout_once->active = false;
    }
    return val;
  }
  return false;
}