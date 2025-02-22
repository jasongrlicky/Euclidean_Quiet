#ifndef CLOCK_H_
#define CLOCK_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "common/events.h"
#include "common/types.h"

/// @brief Respond to input events and populate the passed-in struct with 
/// an internal clock tick event periodically.
/// @param events Input events, to which this function will add an internal
/// clock tick event if one should be generated this cycle.
void internal_clock_update(InputEvents *events, Milliseconds now);

#ifdef __cplusplus
}
#endif
#endif /* CLOCK_H_ */