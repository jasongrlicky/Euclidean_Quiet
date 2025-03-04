#ifndef MODE_STATE_H_
#define MODE_STATE_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "euclid.h"

typedef union ModeState {
	EuclidState euclid;
} ModeState;

#ifdef __cplusplus
}
#endif
#endif /* MODE_STATE_H_ */