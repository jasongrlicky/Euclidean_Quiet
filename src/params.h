#ifndef PARAMS_H_
#define PARAMS_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"

#define EUCLID_PARAMS_NUM 9
#define EUCLID_PARAMS_OFFSET 0

/// A parameter id for the Euclidean rhythm generator mode, which indexes into
/// parameter tables.
typedef enum EuclidParamId {
	EUCLID_PARAM_ID_CH1_LENGTH,
	EUCLID_PARAM_ID_CH1_DENSITY,
	EUCLID_PARAM_ID_CH1_OFFSET,
	EUCLID_PARAM_ID_CH2_LENGTH,
	EUCLID_PARAM_ID_CH2_DENSITY,
	EUCLID_PARAM_ID_CH2_OFFSET,
	EUCLID_PARAM_ID_CH3_LENGTH,
	EUCLID_PARAM_ID_CH3_DENSITY,
	EUCLID_PARAM_ID_CH3_OFFSET,
} EuclidParamId;

#define PARAMS_TOTAL 9

// clang-format off
#if LOGGING_ENABLED
/// Table of parameter names for logging
static const char params_names[PARAMS_TOTAL][3] = {
	// Euclid Mode
	"L1", // Length, Channel 1
	"D1", // Density, Channel 1
	"O1", // Offset, Channel 1
	"L2",
	"D2",
	"O2",
	"L3",
	"D3",
	"O3",
};
#endif
// clang-format on

#ifdef __cplusplus
}
#endif
#endif /* PARAMS_H_ */
