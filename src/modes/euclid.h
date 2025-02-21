#ifndef EUCLID_H_
#define EUCLID_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "common/events.h"
#include "common/timeout.h"
#include "common/types.h"
#include "params.h"

#include <stdbool.h>
#include <stdint.h>

void euclid_params_validate(Params *params);
void euclid_init(void);
void euclid_update(const InputEvents *events, Milliseconds now);

#ifdef __cplusplus
}
#endif
#endif /* EUCLID_H_ */