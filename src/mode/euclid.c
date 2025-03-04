#include "euclid.h"

#include "common/math.h"
#include "config.h"
#include "hardware/output.h"
#include "ui/active_channel.h"
#include "ui/indicators.h"

#include <euclidean.h>

/* CONSTANTS */

static const uint8_t EUCLID_PARAMS_PER_CHANNEL = 3;

// Bounds for three channel parameters
// Length (N)
static const uint8_t PARAM_LENGTH_MIN = 1;
static const uint8_t PARAM_LENGTH_MAX = 16;
static const uint8_t PARAM_LENGTH_DEFAULT = 16;
// Density (K)
static const uint8_t PARAM_DENSITY_MIN = 0;
static const uint8_t PARAM_DENSITY_MAX = 16;
static const uint8_t PARAM_DENSITY_DEFAULT = 4;
// Offset (O)
static const uint8_t PARAM_OFFSET_MIN = 0;
static const uint8_t PARAM_OFFSET_MAX = 15;
static const uint8_t PARAM_OFFSET_DEFAULT = 0;

/* DATA STRUCTURES */

/// The kind of a parameter for a channel of the Euclidean rhythm generator
typedef enum EuclidParam {
	EUCLID_PARAM_LENGTH,
	EUCLID_PARAM_DENSITY,
	EUCLID_PARAM_OFFSET,
} EuclidParam;

/// Parameter for the Euclidean rhythm generator mode that is wrapped as an optional value
typedef struct EuclidParamOpt {
	EuclidParam inner;
	bool valid;
} EuclidParamOpt;

static const EuclidParamOpt EUCLID_PARAM_OPT_NONE = {.inner = EUCLID_PARAM_LENGTH, .valid = false};

// clang-format off
static const EuclidState EUCLID_STATE_INIT = {
    // First channel is selected on init
    .active_channel = CHANNEL_1,
    .generated_rhythms = {0, 0, 0},
    .sequencer = {
			.positions = {0, 0, 0},
			.running = false,
		},
		.adjustment_display = {
			.timeout = {.duration = ADJUSTMENT_DISPLAY_TIME},
			.channel = CHANNEL_1,	
			.visible = false,
		},
		.output_pulse = {
			.timeout = {.inner = {.duration = 5}}, 
			.last_clock_or_reset= 0,
		},
		.playhead = {
			.flash_timeout = {.inner = {.duration = PLAYHEAD_FLASH_TIME_DEFAULT}},
			.idle_timeout = {.duration = PLAYHEAD_IDLE_TIME},
			.idle_loop_timeout = {.duration = PLAYHEAD_IDLE_LOOP_PERIOD},
		},
};
// clang-format on

/* DECLARATIONS */

static void euclid_handle_encoder_push(EuclidState *state, EncoderIdx enc_idx);
static EuclidParamOpt euclid_handle_encoder_move(EuclidState *state, Params *params, const int16_t *enc_move);
// Returns bitflags storing which output channels will fire this cycle, indexed
// by `OutputChannel`.
static uint8_t euclid_update_sequencers(EuclidState *state, const Params *params, const InputEvents *events);
static void sequencer_handle_reset(EuclidState *state);
static void sequencer_handle_clock(EuclidState *state, const Params *params);
static void sequencer_advance(EuclidState *state, const Params *params);
/// What is the output that should be sent for each sequencer this cycle
/// @return Bitflags, indexed using `OutputChannel`. 1 = begin an output pulse this cycle for this channel, 0
/// = do nothing for this channel
static uint8_t sequencer_read_current_step(EuclidState *state, const Params *params);
static void euclid_draw_channels(const EuclidState *state, Framebuffer *fb, const Params *params);
static inline void draw_channel(const EuclidState *state, Framebuffer *fb, Channel channel, uint8_t length);
static inline void draw_channel_length(Framebuffer *fb, Channel channel, uint16_t pattern, uint8_t length);
static inline void draw_channel_pattern(const EuclidState *state, Framebuffer *fb, Channel channel,
                                        uint16_t pattern, uint8_t length, uint8_t position);
/// Read a single step from a pattern
/// @param pattern The pattern to read from, stored as 16 bitflags.
/// @param length The length of the pattern. Must be <= 16.
/// @param position The step at which to read. Must be < `length`.
/// @return `true` if there is an active step at this position, `false` otherwise.
static bool pattern_read(uint16_t pattern, uint8_t length, uint8_t position);
static Milliseconds calc_playhead_flash_time(Milliseconds clock_period);
static ChannelOpt channel_for_encoder(EncoderIdx enc_idx);
/// Wrap the provided value as an occupied optional
static inline EuclidParamOpt euclid_param_opt(EuclidParam inner);
/// Return the `ParamIdx` for a given a channel and param kind
static inline ParamIdx euclid_param_idx(Channel channel, EuclidParam kind);
static inline uint8_t euclid_param_get(const Params *params, Channel channel, EuclidParam kind);
static inline void euclid_param_set(Params *params, Channel channel, EuclidParam kind, uint8_t val);
static inline uint8_t euclid_get_length(const Params *params, Channel channel);
static inline uint8_t euclid_get_density(const Params *params, Channel channel);
static inline uint8_t euclid_get_offset(const Params *params, Channel channel);

/* EXTERNAL */

void euclid_params_validate(Params *params) {
	for (uint8_t c = 0; c < NUM_CHANNELS; c++) {
		Channel channel = (Channel)c;
		uint8_t length = euclid_get_length(params, channel);
		uint8_t density = euclid_get_density(params, channel);
		uint8_t offset = euclid_get_offset(params, channel);

		if ((length > PARAM_LENGTH_MAX) || (length < PARAM_LENGTH_MIN)) {
			euclid_param_set(params, channel, EUCLID_PARAM_LENGTH, PARAM_LENGTH_DEFAULT);
		}
		if (density > PARAM_DENSITY_MAX || density > length) {
			euclid_param_set(params, channel, EUCLID_PARAM_DENSITY, PARAM_DENSITY_DEFAULT);
		}
		if (offset > PARAM_OFFSET_MAX || offset > length) {
			euclid_param_set(params, channel, EUCLID_PARAM_OFFSET, PARAM_OFFSET_DEFAULT);
		}
	}
}

void euclid_init(EuclidState *state, const Params *params, Framebuffer *fb) {
	*state = EUCLID_STATE_INIT;

	// Initialise generated rhythms based on params
	for (int a = 0; a < NUM_CHANNELS; a++) {
		const Channel channel = (Channel)a;
		const uint8_t length = euclid_get_length(params, channel);
		const uint8_t density = euclid_get_density(params, channel);
		const uint8_t offset = euclid_get_offset(params, channel);
		state->generated_rhythms[a] = euclidean_pattern_rotate(length, density, offset);
	}

	// Draw initial UI
	euclid_draw_channels(state, fb, params);
	active_channel_display_draw(fb, state->active_channel);
}

void euclid_update(EuclidState *state, Params *params, Framebuffer *fb, const InputEvents *events,
                   Milliseconds now) {
	euclid_handle_encoder_push(state, events->enc_push);

	// Note the param associated with a knob that was moved so we can re-generate
	// the Euclidean rhythms and show the adjustment display.
	const EuclidParamOpt param_knob_moved = euclid_handle_encoder_move(state, params, events->enc_move);

	// Update Generated Rhythms Based On Parameter Changes
	Channel active_channel = state->active_channel;
	if (param_knob_moved.valid) {
		const Channel channel = active_channel;
		const uint8_t length = euclid_get_length(params, channel);
		const uint8_t density = euclid_get_density(params, channel);
		const uint8_t offset = euclid_get_offset(params, channel);

		state->generated_rhythms[channel] = euclidean_pattern_rotate(length, density, offset);
	}

	/* UPDATE SEQUENCER */

	// Clock ticks merge the internal and external clocks
	const bool clock_tick = events->trig || events->internal_clock_tick;

	// Tracks if any of the sequencers' states have been updated this cycle
	const bool sequencers_updated = (clock_tick || events->reset);

	// Bitflags storing which output channels will fire this cycle, indexed by
	// `OutputChannel`.
	const uint8_t out_channels_firing = euclid_update_sequencers(state, params, events);

	/* OUTPUT */

	for (uint8_t out_channel = 0; out_channel < OUTPUT_NUM_CHANNELS; out_channel++) {
		const bool should_fire = out_channels_firing & (0x01 << out_channel);
		if (should_fire) {
			output_set_high((OutputChannel)out_channel);
		}
	}

	if (sequencers_updated) {
		// Update output pulse length and timeout
		const Milliseconds time_since_last = now - state->output_pulse.timeout.inner.start;
		const Milliseconds pulse_length = CONSTRAIN(time_since_last / 5, 2, 5);
		state->output_pulse.timeout.inner.duration = pulse_length;

		timeout_once_reset(&state->output_pulse.timeout, now);
	}

	// FINISH ANY PULSES THAT ARE ACTIVE
	if (timeout_once_fired(&state->output_pulse.timeout, now)) {
		output_clear_all();
	}

	/* DRAWING - ACTIVE CHANNEL DISPLAY */

	if (events->enc_push != ENCODER_NONE) {
		active_channel_display_draw(fb, active_channel);
	}

	/* DRAWING - CHANNELS */

	if (sequencers_updated) {
		// Update playhead flash duration based on the last interval between two
		// clock or reset signals received.
		const Milliseconds previous_period = now - state->output_pulse.last_clock_or_reset;
		state->playhead.flash_timeout.inner.duration = calc_playhead_flash_time(previous_period);
		state->output_pulse.last_clock_or_reset = now;

		// Reset playhead flash
		timeout_once_reset(&state->playhead.flash_timeout, now);

		// Reset playhead idle
		timeout_reset(&state->playhead.idle_timeout, now);
	}

	// Update playhead idle - Make playhead flash periodically when it hasn't
	// moved in a certain amount of time
	bool playhead_flash_updated = false;
	if (timeout_fired(&state->playhead.idle_timeout, now)) {
		if (timeout_loop(&state->playhead.idle_loop_timeout, now)) {
			state->playhead.flash_timeout.inner.duration = PLAYHEAD_FLASH_TIME_DEFAULT;
			timeout_once_reset(&state->playhead.flash_timeout, now);
			playhead_flash_updated = true;
		}
	}

	// Update playhead flash
	if (timeout_once_fired(&state->playhead.flash_timeout, now)) {
		playhead_flash_updated = true;
	}

	// Tracks if the screen needs to be redrawn.
	bool needs_redraw = sequencers_updated || playhead_flash_updated;

	if (param_knob_moved.valid) {
		if (param_knob_moved.inner == EUCLID_PARAM_LENGTH) {
			// If length parameter was changed, reset the adjustment display timeout and state
			state->adjustment_display.channel = active_channel;
			state->adjustment_display.visible = true;
			timeout_reset(&state->adjustment_display.timeout, now);
		} else {
			// Otherwise, just hide the adjustment display
			state->adjustment_display.visible = false;
		}

		needs_redraw = true;
	} else {
		// If no parameters have changed, check if the adjustment display still
		// needs to be shown, and hide it if it doesn't
		if (state->adjustment_display.visible) {
			bool should_be_hidden = timeout_fired(&state->adjustment_display.timeout, now);
			if (should_be_hidden) {
				state->adjustment_display.visible = false;
				needs_redraw = true;
			}
		}
	}

	if (needs_redraw) {
		euclid_draw_channels(state, fb, params);
	}

	/* DRAWING - OUTPUT INDICATORS */

	if (sequencers_updated) {
		indicators_output_latching_draw(fb, out_channels_firing);
	}
}

/* INTERNAL */

static void euclid_handle_encoder_push(EuclidState *state, EncoderIdx enc_idx) {
	const ChannelOpt active_channel_new = channel_for_encoder(enc_idx);
	if (active_channel_new.valid) {
		state->active_channel = active_channel_new.inner;
	}
}

static EuclidParamOpt euclid_handle_encoder_move(EuclidState *state, Params *params,
                                                 const int16_t *enc_move) {
	EuclidParamOpt param_knob_moved = EUCLID_PARAM_OPT_NONE;

	const Channel active_channel = state->active_channel;
	const ParamIdx length_idx = euclid_param_idx(active_channel, EUCLID_PARAM_LENGTH);
	const ParamIdx density_idx = euclid_param_idx(active_channel, EUCLID_PARAM_DENSITY);
	const ParamIdx offset_idx = euclid_param_idx(active_channel, EUCLID_PARAM_OFFSET);

	// Handle Length Knob Movement
	int nknob = enc_move[ENCODER_1];
	if (nknob != 0) {
		param_knob_moved = euclid_param_opt(EUCLID_PARAM_LENGTH);

		const Channel channel = active_channel;
		int length = euclid_get_length(params, channel);
		uint8_t density = euclid_get_density(params, channel);
		uint8_t offset = euclid_get_offset(params, channel);
		const uint8_t position = state->sequencer.positions[channel];

		// Keep length in bounds
		if (length >= PARAM_LENGTH_MAX) {
			length = PARAM_LENGTH_MAX;
		}
		if (length + nknob > PARAM_LENGTH_MAX) {
			nknob = 0;
		}
		if (length + nknob < PARAM_LENGTH_MIN) {
			nknob = 0;
		}

		// Reduce density and offset to remain in line with the new length if necessary
		if ((density >= (length + nknob)) && (density > 1)) {
			density += nknob;
			param_and_flags_set(params, density_idx, density);
		}
		if ((offset >= (length + nknob)) && (offset < 16)) {
			offset += nknob;
			param_and_flags_set(params, offset_idx, offset);
		}

		length += nknob;
		param_and_flags_set(params, length_idx, length);

		// Reset position if length has been reduced past it
		if (position >= length) {
			state->sequencer.positions[channel] = 0;
		}
	}

	// Handle Density Knob Movement
	int kknob = enc_move[ENCODER_2];
	if (kknob != 0) {
		param_knob_moved = euclid_param_opt(EUCLID_PARAM_DENSITY);

		const Channel channel = active_channel;
		const int length = euclid_get_length(params, channel);
		uint8_t density = euclid_get_density(params, channel);

		// Keep density in bounds
		if (density + kknob > length) {
			kknob = 0;
		}
		if (density + kknob < PARAM_DENSITY_MIN) {
			kknob = 0;
		}

		density += kknob;
		param_and_flags_set(params, density_idx, density);
	}

	// Handle Offset Knob Movement
	int oknob = enc_move[ENCODER_3];
	if (oknob != 0) {
		param_knob_moved = euclid_param_opt(EUCLID_PARAM_OFFSET);

		const Channel channel = active_channel;
		const int length = euclid_get_length(params, channel);
		uint8_t offset = euclid_get_offset(params, channel);

		// Keep offset in bounds
		if (offset + oknob > length - 1) {
			oknob = 0;
		}
		if (offset + oknob < PARAM_OFFSET_MIN) {
			oknob = 0;
		}

		offset += oknob;
		param_and_flags_set(params, offset_idx, offset);
	}

	return param_knob_moved;
}

static uint8_t euclid_update_sequencers(EuclidState *state, const Params *params, const InputEvents *events) {
	// Clock ticks merge the internal and external clocks
	const bool clock_tick = events->trig || events->internal_clock_tick;

	uint8_t out_channels_firing = 0;

	if (events->reset) {
		sequencer_handle_reset(state);
	}

	if (clock_tick) {
		sequencer_handle_clock(state, params);

		out_channels_firing = sequencer_read_current_step(state, params);
	}

	return out_channels_firing;
}

static void sequencer_handle_reset(EuclidState *state) {
	// Go to the first step for each channel
	for (uint8_t channel = 0; channel < NUM_CHANNELS; channel++) {
		state->sequencer.positions[channel] = 0;
	}

	// Stop the sequencer
	state->sequencer.running = false;
}

static void sequencer_handle_clock(EuclidState *state, const Params *params) {
	// Advance sequencer if it is running
	if (state->sequencer.running) {
		// Only advance if sequencer is running
		sequencer_advance(state, params);
	} else {
		// If sequencer is stopped, start it so that the next clock advances
		state->sequencer.running = true;
	}
}

static void sequencer_advance(EuclidState *state, const Params *params) {
	for (uint8_t channel = 0; channel < NUM_CHANNELS; channel++) {
		uint8_t position = state->sequencer.positions[channel];
		const uint8_t length = euclid_get_length(params, channel);

		// Move sequencer playhead to next step
		position++;
		if (position >= length) {
			position = 0;
		}
		state->sequencer.positions[channel] = position;
	}
}

static uint8_t sequencer_read_current_step(EuclidState *state, const Params *params) {
	uint8_t out_channels_firing = 0;

	for (uint8_t channel = 0; channel < NUM_CHANNELS; channel++) {
		const uint8_t length = euclid_get_length(params, channel);
		const uint8_t position = state->sequencer.positions[channel];
		const uint16_t pattern = state->generated_rhythms[channel];

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

static void euclid_draw_channels(const EuclidState *state, Framebuffer *fb, const Params *params) {
	for (uint8_t channel = 0; channel < NUM_CHANNELS; channel++) {
		const uint8_t length = euclid_get_length(params, channel);
		draw_channel(state, fb, (Channel)channel, length);
	}
}

static inline void draw_channel(const EuclidState *state, Framebuffer *fb, Channel channel, uint8_t length) {
	const uint8_t position = state->sequencer.positions[channel];
	const uint16_t pattern = state->generated_rhythms[channel];

	draw_channel_pattern(state, fb, channel, pattern, length, position);

	const bool showing_length_display =
	    (state->adjustment_display.visible) && (channel == state->adjustment_display.channel);
	if (showing_length_display) {
		draw_channel_length(fb, channel, pattern, length);
	}
}

static inline void draw_channel_length(Framebuffer *fb, Channel channel, uint16_t pattern, uint8_t length) {
	const uint8_t row = channel * 2;

	for (uint8_t step = length; step < 16; step++) {
		uint8_t x = step;
		uint8_t y = row;
		if (step > 7) {
			x -= 8;
			y += 1;
		}

		framebuffer_pixel_set_fast(fb, x, y, COLOR_ANTS);
	}
}

static inline void draw_channel_pattern(const EuclidState *state, Framebuffer *fb, Channel channel,
                                        uint16_t pattern, uint8_t length, uint8_t position) {
	const bool playhead_flash_active = state->playhead.flash_timeout.active;

	uint16_t pixel_rows[2] = {0, 0};

	// Optimization - The do/while loop, decrementing iteration, and separate
	// counter for steps are all faster than otherwise.
	uint8_t i = length;
	uint8_t step = length - 1;
	do {
		// Optimization - Read current step and shift pattern for next loop. Faster
		// than pattern_read because index and mask aren't recalculated.
		const bool active_step = pattern & 0x01;
		pattern >>= 1;

		const bool playhead_here = (step == position);
		const bool flashing_now = playhead_here && playhead_flash_active;
		Color color;
		if (flashing_now) {
			color = COLOR_BLINK;
		} else {
			color = (active_step) ? COLOR_ON : COLOR_OFF;
		}

		if (step < 8) {
			pixel_rows[0] <<= 2;
			pixel_rows[0] |= color;
		} else {
			pixel_rows[1] <<= 2;
			pixel_rows[1] |= color;
		}

		step--;
		i--;
	} while (i);

	const uint8_t row = channel * 2;
	framebuffer_row_set(fb, row, pixel_rows[0]);
	framebuffer_row_set(fb, row + 1, pixel_rows[1]);
}

static bool pattern_read(uint16_t pattern, uint8_t length, uint8_t position) {
	const uint8_t idx = length - position - 1;
	return (pattern >> idx) & 0x01;
}

static Milliseconds calc_playhead_flash_time(Milliseconds clock_period) {
	// This is a standard "scale from input range to output range" function, but
	// it uses specific ranges so that we can avoid multiplication or division by
	// numbers that aren't powers of 2.

	// 256ms min period = ~234bpm
	// 1280ms max period = ~47bpm
	// 1280-256 = an input range of 1024, or 2^10
	clock_period = CONSTRAIN(clock_period, 256, 1280);
	// Subtract input min
	const Milliseconds delta = clock_period - 256;
	// (delta / input range) * output range. Input range is 2^10, output range is
	// 2^7, so just divide by 2^3.
	Milliseconds result = delta >> 3;
	// Add output min
	result += 64;
	return result;
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

static inline EuclidParamOpt euclid_param_opt(EuclidParam inner) {
	return (EuclidParamOpt){.inner = inner, .valid = true};
}

static inline ParamIdx euclid_param_idx(Channel channel, EuclidParam kind) {
	return (ParamIdx)((channel * EUCLID_PARAMS_PER_CHANNEL) + kind);
}

static inline uint8_t euclid_param_get(const Params *params, Channel channel, EuclidParam kind) {
	ParamIdx idx = euclid_param_idx(channel, kind);
	return params->values[idx];
}

static inline void euclid_param_set(Params *params, Channel channel, EuclidParam kind, uint8_t val) {
	const ParamIdx idx = euclid_param_idx(channel, kind);
	params->values[idx] = val;
}

static inline uint8_t euclid_get_length(const Params *params, Channel channel) {
	return euclid_param_get(params, channel, EUCLID_PARAM_LENGTH);
}

static inline uint8_t euclid_get_density(const Params *params, Channel channel) {
	return euclid_param_get(params, channel, EUCLID_PARAM_DENSITY);
}

static inline uint8_t euclid_get_offset(const Params *params, Channel channel) {
	return euclid_param_get(params, channel, EUCLID_PARAM_OFFSET);
}
