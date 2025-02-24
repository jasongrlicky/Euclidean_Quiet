#ifndef EUCLID_H_
#define EUCLID_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "common/events.h"
#include "common/params.h"
#include "common/types.h"
#include "ui/framebuffer.h"

#include <stdbool.h>
#include <stdint.h>

void euclid_params_validate(Params *params);
void euclid_init(const Params *params, Framebuffer *fb);
void euclid_update(Params *params, Framebuffer *fb, const InputEvents *events, Milliseconds now);

#ifdef __cplusplus
}
#endif
#endif /* EUCLID_H_ */