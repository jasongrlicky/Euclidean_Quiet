#include "timeout.h"

void timeout_reset(Timeout *timeout, Milliseconds now) {
  timeout->start = now;
}

bool timeout_fired(Timeout const *timeout, Milliseconds now) {
  return ((now - timeout->start) >= timeout->duration);
}
