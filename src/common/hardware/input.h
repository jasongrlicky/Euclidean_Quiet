#ifndef INPUT_H_
#define INPUT_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "events.h"
#include "types.h"

/// Populates the passed-in struct with events observed since last cycle.
void input_update(InputEvents *events, Milliseconds now);

#ifdef __cplusplus
}
#endif
#endif /* INPUT_H_ */
