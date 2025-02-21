#include "mode_params.h"

#include "modes/euclid.h"

void mode_params_validate(Params *params, Mode mode) {
	switch (mode) {
		case MODE_EUCLID:
			euclid_params_validate(params);
			break;
	}
}
