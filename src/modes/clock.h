#ifndef CLOCK_H_
#define CLOCK_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "common/events.h"
#include "common/types.h"

/// @brief Update internal clock mode, responding to input events and generating 
/// a clock tick periodically
/// @param events Input events, to which this function will add an internal
/// clock tick event if one should be generated this cycle.
void internal_clock_update(InputEvents *events, Milliseconds now);

#ifdef __cplusplus
}
#endif
#endif /* CLOCK_H_ */