#include "mode.h"

#include "modes/euclid.h"

void mode_init(Mode mode) {
	switch (mode) {
		case MODE_EUCLID:
			euclid_init();
			break;
	}
}

void mode_update(Mode mode, const InputEvents *events, Milliseconds now) {
	switch (mode) {
		case MODE_EUCLID:
			euclid_update(events, now);
			break;
	}
}
