#ifndef MODE_H_
#define MODE_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "common/events.h"
#include "common/types.h"

#define NUM_MODES 1
typedef enum Mode {
	MODE_EUCLID,
} Mode;

void mode_init(Mode mode);
void mode_update(Mode mode, const InputEvents *events, Milliseconds now);

#ifdef __cplusplus
}
#endif
#endif /* MODE_H_ */
