#include "euclid.h"

#include "config.h"
#include "ui/framebuffer.h"

/* GLOBALS */

EuclidState euclid_state = {
    .active_channel = CHANNEL_1,
    .sequencer_positions = {0, 0, 0},
    .sequencer_running = false,
};

/// Stores each generated Euclidean rhythm as 16 bits. Indexed by channel number.
uint16_t generated_rhythms[NUM_CHANNELS];

AdjustmentDisplayState adjustment_display_state = {
    .channel = CHANNEL_1,
    .parameter = EUCLID_PARAM_LENGTH,
    .visible = false,
};

// Tracks the playhead flash itself
TimeoutOnce playhead_flash_timeout = {.inner = {.duration = PLAYHEAD_FLASH_TIME_DEFAULT}};

/* DECLARATIONS */

static void sequencer_handle_reset();
static void sequencer_handle_clock();
static void sequencer_advance();
/// What is the output that should be sent for each sequencer this cycle
/// @return Bitflags, indexed using `OutputChannel`. 1 = begin an output pulse this cycle for this channel, 0
/// = do nothing for this channel
static uint8_t sequencer_read_current_step();
static inline void draw_channel(Channel channel);
static inline void draw_channel_length(Channel channel, uint16_t pattern, uint8_t length);
static inline void draw_channel_pattern(Channel channel, uint16_t pattern, uint8_t length, uint8_t position);
/// Read a single step from a pattern
/// @param pattern The pattern to read from, stored as 16 bitflags.
/// @param length The length of the pattern. Must be <= 16.
/// @param position The step at which to read. Must be < `length`.
/// @return `true` if there is an active step at this position, `false` otherwise.
static bool pattern_read(uint16_t pattern, uint8_t length, uint8_t position);
static ChannelOpt channel_for_encoder(EncoderIdx enc_idx);

/* EXTERNAL */

void euclid_params_validate(Params *params) {
	for (uint8_t c = 0; c < NUM_CHANNELS; c++) {
		Channel channel = (Channel)c;
		uint8_t length = euclid_get_length(params, channel);
		uint8_t density = euclid_get_density(params, channel);
		uint8_t offset = euclid_get_offset(params, channel);

		if ((length > BEAT_LENGTH_MAX) || (length < BEAT_LENGTH_MIN)) {
			euclid_param_set(params, channel, EUCLID_PARAM_LENGTH, BEAT_LENGTH_DEFAULT);
		}
		if (density > BEAT_DENSITY_MAX || density > length) {
			euclid_param_set(params, channel, EUCLID_PARAM_DENSITY, BEAT_DENSITY_DEFAULT);
		}
		if (offset > BEAT_OFFSET_MAX || offset > length) {
			euclid_param_set(params, channel, EUCLID_PARAM_OFFSET, BEAT_OFFSET_DEFAULT);
		}
	}
}

EuclidParamOpt euclid_param_opt(EuclidParam inner) { return (EuclidParamOpt){.inner = inner, .valid = true}; }

EuclidParamOpt euclid_param_for_encoder(EncoderIdx enc_idx) {
	switch (enc_idx) {
		case ENCODER_1:
			return (EuclidParamOpt){.inner = EUCLID_PARAM_LENGTH, .valid = true};
			break;
		case ENCODER_2:
			return (EuclidParamOpt){.inner = EUCLID_PARAM_DENSITY, .valid = true};
			break;
		case ENCODER_3:
			return (EuclidParamOpt){.inner = EUCLID_PARAM_OFFSET, .valid = true};
			break;
		default:
			return EUCLID_PARAM_OPT_NONE;
			break;
	}
}

void euclid_handle_encoder_push(EncoderIdx enc_idx) {
	ChannelOpt active_channel_new = channel_for_encoder(enc_idx);
	if (active_channel_new.valid) {
		euclid_state.active_channel = active_channel_new.inner;
	}
}

EuclidParamOpt euclid_handle_encoder_move(const int16_t *enc_move) {
	EuclidParamOpt param_knob_moved = EUCLID_PARAM_OPT_NONE;

	Channel active_channel = euclid_state.active_channel;
	ParamIdx length_idx = euclid_param_idx(active_channel, EUCLID_PARAM_LENGTH);
	ParamIdx density_idx = euclid_param_idx(active_channel, EUCLID_PARAM_DENSITY);
	ParamIdx offset_idx = euclid_param_idx(active_channel, EUCLID_PARAM_OFFSET);

	// Handle Length Knob Movement
	int nknob = enc_move[ENCODER_1];
	if (nknob != 0) {
		param_knob_moved = euclid_param_opt(EUCLID_PARAM_LENGTH);

		Channel channel = active_channel;
		int length = euclid_get_length(&params, channel);
		uint8_t density = euclid_get_density(&params, channel);
		uint8_t offset = euclid_get_offset(&params, channel);
		uint8_t position = euclid_state.sequencer_positions[channel];

		// Keep length in bounds
		if (length >= BEAT_LENGTH_MAX) {
			length = BEAT_LENGTH_MAX;
		}
		if (length + nknob > BEAT_LENGTH_MAX) {
			nknob = 0;
		}
		if (length + nknob < BEAT_LENGTH_MIN) {
			nknob = 0;
		}

		// Reduce density and offset to remain in line with the new length if necessary
		if ((density >= (length + nknob)) && (density > 1)) {
			density += nknob;

			param_and_flags_set(&params, density_idx, density);
		}
		if ((offset >= (length + nknob)) && (offset < 16)) {
			offset += nknob;

			param_and_flags_set(&params, offset_idx, offset);
		}

		length += nknob;

		param_and_flags_set(&params, length_idx, length);

		// Reset position if length has been reduced past it
		if (position >= length) {
			euclid_state.sequencer_positions[channel] = 0;
		}
	}

	// Handle Density Knob Movement
	int kknob = enc_move[ENCODER_2];
	if (kknob != 0) {
		param_knob_moved = euclid_param_opt(EUCLID_PARAM_DENSITY);

		Channel channel = active_channel;
		int length = euclid_get_length(&params, channel);
		uint8_t density = euclid_get_density(&params, channel);

		// Keep density in bounds
		if (density + kknob > length) {
			kknob = 0;
		}
		if (density + kknob < BEAT_DENSITY_MIN) {
			kknob = 0;
		}

		density += kknob;

		param_and_flags_set(&params, density_idx, density);
	}

	// Handle Offset Knob Movement
	int oknob = enc_move[ENCODER_3];
	if (oknob != 0) {
		param_knob_moved = euclid_param_opt(EUCLID_PARAM_OFFSET);

		Channel channel = active_channel;
		int length = euclid_get_length(&params, channel);
		uint8_t offset = euclid_get_offset(&params, channel);

		// Keep offset in bounds
		if (offset + oknob > length - 1) {
			oknob = 0;
		}
		if (offset + oknob < BEAT_OFFSET_MIN) {
			oknob = 0;
		}

		offset += oknob;

		param_and_flags_set(&params, offset_idx, offset);
	}

	return param_knob_moved;
}

uint8_t euclid_update(const InputEvents *events) {
	// Clock ticks merge the internal and external clocks
	bool clock_tick = events->trig || events->internal_clock_tick;

	uint8_t out_channels_firing = 0;

	if (events->reset) {
		sequencer_handle_reset();
	}

	if (clock_tick) {
		sequencer_handle_clock();

		out_channels_firing = sequencer_read_current_step();
	}

	return out_channels_firing;
}

void euclid_draw_channels(void) {
	for (uint8_t channel = 0; channel < NUM_CHANNELS; channel++) {
		draw_channel((Channel)channel);
	}
}

/* INTERNAL */

static void sequencer_handle_reset() {
	// Go to the first step for each channel
	for (uint8_t channel = 0; channel < NUM_CHANNELS; channel++) {
		euclid_state.sequencer_positions[channel] = 0;
	}

	// Stop the sequencer
	euclid_state.sequencer_running = false;
}

static void sequencer_handle_clock() {
	// Advance sequencer if it is running
	if (euclid_state.sequencer_running) {
		// Only advance if sequencer is running
		sequencer_advance();
	} else {
		// If sequencer is stopped, start it so that the next clock advances
		euclid_state.sequencer_running = true;
	}
}

static void sequencer_advance() {
	for (uint8_t channel = 0; channel < NUM_CHANNELS; channel++) {
		uint8_t position = euclid_state.sequencer_positions[channel];
		uint8_t length = euclid_get_length(&params, channel);

		// Move sequencer playhead to next step
		position++;
		if (position >= length) {
			position = 0;
		}
		euclid_state.sequencer_positions[channel] = position;
	}
}

static uint8_t sequencer_read_current_step() {
	uint8_t out_channels_firing = 0;

	for (uint8_t channel = 0; channel < NUM_CHANNELS; channel++) {
		uint8_t length = euclid_get_length(&params, channel);
		uint8_t position = euclid_state.sequencer_positions[channel];
		uint16_t pattern = generated_rhythms[channel];

		// Turn on LEDs on the bottom row for channels where the step is active
		bool step_is_active = pattern_read(pattern, length, position);
		if (step_is_active) {
			out_channels_firing |= (0x01 << channel);
		} else {
			// Create output pulses for Offbeat Channel - inverse of Channel 1
			if (channel == CHANNEL_1) {
				out_channels_firing |= (0x01 << OUTPUT_CHANNEL_OFFBEAT);
			}
		}
	}

	return out_channels_firing;
}

static inline void draw_channel(Channel channel) {
	uint8_t length = euclid_get_length(&params, channel);
	uint8_t position = euclid_state.sequencer_positions[channel];
	uint16_t pattern = generated_rhythms[channel];

	// Clear rows
	uint8_t row = channel * 2;
	framebuffer_row_off(row);
	framebuffer_row_off(row + 1);

	bool showing_length_display = (adjustment_display_state.visible) &&
	                              (channel == adjustment_display_state.channel) &&
	                              (adjustment_display_state.parameter == EUCLID_PARAM_LENGTH);
	if (showing_length_display) {
		draw_channel_length(channel, pattern, length);
	}
	draw_channel_pattern(channel, pattern, length, position);
}

static inline void draw_channel_length(Channel channel, uint16_t pattern, uint8_t length) {
	uint8_t row = channel * 2;

	for (uint8_t step = length; step < 16; step++) {
		uint8_t x = step;
		uint8_t y = row;
		if (step > 7) {
			x -= 8;
			y += 1;
		}

		framebuffer_pixel_set_fast(x, y, COLOR_ANTS);
	}
}

static inline void draw_channel_pattern(Channel channel, uint16_t pattern, uint8_t length, uint8_t position) {
	uint8_t row = channel * 2;

	for (uint8_t step = 0; step < length; step++) {
		uint8_t x = step;
		uint8_t y = row;
		if (step > 7) {
			x -= 8;
			y += 1;
		}

		bool active_step = pattern_read(pattern, length, step);
		bool playhead_here = (step == position);
		bool playhead_flash_active = playhead_flash_timeout.active;
		bool flashing_now = playhead_here && playhead_flash_active;
		Color color;
		if (flashing_now) {
			color = COLOR_BLINK;
		} else {
			color = (active_step) ? COLOR_ON : COLOR_OFF;
		}

		framebuffer_pixel_set_fast(x, y, color);
	}
}

static bool pattern_read(uint16_t pattern, uint8_t length, uint8_t position) {
	uint8_t idx = length - position - 1;
	return (pattern >> idx) & 0x01;
}

static ChannelOpt channel_for_encoder(EncoderIdx enc_idx) {
	switch (enc_idx) {
		case ENCODER_1:
			return (ChannelOpt){.inner = CHANNEL_2, .valid = true};
			break;
		case ENCODER_2:
			return (ChannelOpt){.inner = CHANNEL_3, .valid = true};
			break;
		case ENCODER_3:
			return (ChannelOpt){.inner = CHANNEL_1, .valid = true};
			break;
		default:
			return CHANNEL_OPT_NONE;
			break;
	}
}