#include "euclid.h"

#include "common/math.h"
#include "common/timeout.h"
#include "config.h"
#include "hardware/output.h"
#include "ui/active_channel.h"
#include "ui/framebuffer.h"
#include "ui/indicators.h"

#include <euclidean.h>

/* CONSTANTS */

#define NUM_CHANNELS 3
#define EUCLID_PARAMS_PER_CHANNEL 3

// Bounds for three channel parameters
// N: Beat Length
#define BEAT_LENGTH_MIN 1
#define BEAT_LENGTH_MAX 16
#define BEAT_LENGTH_DEFAULT 16
// K: Density
#define BEAT_DENSITY_MIN 0
#define BEAT_DENSITY_MAX 16
#define BEAT_DENSITY_DEFAULT 4
// O: Offset
#define BEAT_OFFSET_MIN 0
#define BEAT_OFFSET_MAX 15
#define BEAT_OFFSET_DEFAULT 0
// Sequencer position
#define BEAT_POSITION_MAX 15
#define BEAT_POSITION_DEFAULT 0

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

/// State of the entire Euclidean rhythm generator mode
typedef struct EuclidState {
	/// The sequencer channel that is currently selected
	Channel active_channel;
	/// Step index representing the playhead position for for each of this mode's
	/// channels, indexed by `Channel` enum. Valid values are `0` to `15`.
	uint8_t sequencer_positions[NUM_CHANNELS];
	bool sequencer_running;
} EuclidState;

typedef struct AdjustmentDisplayState {
	/// Which channel is currently showing its adjustment display. Only one
	/// adjustment display can be visible at a time.
	Channel channel;
	/// The parameter that is being displayed in the adjustment display.
	EuclidParam parameter;
	/// Is the adjustment display showing currently
	bool visible;
} AdjustmentDisplayState;

/* GLOBALS */

static EuclidState euclid_state = {
    .active_channel = CHANNEL_1,
    .sequencer_positions = {0, 0, 0},
    .sequencer_running = false,
};

/// Stores each generated Euclidean rhythm as 16 bits. Indexed by channel number.
static uint16_t generated_rhythms[NUM_CHANNELS];

static AdjustmentDisplayState adjustment_display_state = {
    .channel = CHANNEL_1,
    .parameter = EUCLID_PARAM_LENGTH,
    .visible = false,
};

static Milliseconds last_clock_or_reset;

static TimeoutOnce output_pulse_timeout = {
    .inner = {.duration = 5}}; // Pulse length, set based on the time since last trigger

// Tracks the playhead flash itself
static TimeoutOnce playhead_flash_timeout = {.inner = {.duration = PLAYHEAD_FLASH_TIME_DEFAULT}};
// Track the time since the playhead has moved so we can make it flash in its idle loop
static Timeout playhead_idle_timeout = {.duration = PLAYHEAD_IDLE_TIME};
// Loop for making the playhead flash periodically after it is idle
static Timeout playhead_idle_loop_timeout = {.duration = PLAYHEAD_IDLE_LOOP_PERIOD};

static Timeout adjustment_display_timeout = {.duration = ADJUSTMENT_DISPLAY_TIME};

/* DECLARATIONS */

static void euclid_handle_encoder_push(EncoderIdx enc_idx);
static EuclidParamOpt euclid_handle_encoder_move(const int16_t *enc_move);
// Returns bitflags storing which output channels will fire this cycle, indexed
// by `OutputChannel`.
static uint8_t euclid_update_sequencers(const InputEvents *events);
static void euclid_draw_channels(void);
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

void euclid_init(void) {
	// Initialise generated rhythms
	for (int a = 0; a < NUM_CHANNELS; a++) {
		Channel channel = (Channel)a;
		uint8_t length = euclid_get_length(&params, channel);
		uint8_t density = euclid_get_density(&params, channel);
		uint8_t offset = euclid_get_offset(&params, channel);
		generated_rhythms[a] = euclidean_pattern_rotate(length, density, offset);
	}

	// Select first channel on startup
	euclid_state.active_channel = CHANNEL_1;

	// Draw initial UI
	euclid_draw_channels();
	active_channel_display_draw(euclid_state.active_channel);
}

void euclid_update(const InputEvents *events, Milliseconds now) {
	euclid_handle_encoder_push(events->enc_push);

	// Note the param associated with a knob that was moved so we can re-generate
	// the Euclidean rhythms and show the adjustment display.
	EuclidParamOpt param_knob_moved = euclid_handle_encoder_move(events->enc_move);

	// Update Generated Rhythms Based On Parameter Changes
	Channel active_channel = euclid_state.active_channel;
	if (param_knob_moved.valid) {
		Channel channel = active_channel;
		uint8_t length = euclid_get_length(&params, channel);
		uint8_t density = euclid_get_density(&params, channel);
		uint8_t offset = euclid_get_offset(&params, channel);

		generated_rhythms[channel] = euclidean_pattern_rotate(length, density, offset);
	}

	/* UPDATE SEQUENCER */

	// Clock ticks merge the internal and external clocks
	bool clock_tick = events->trig || events->internal_clock_tick;

	// Tracks if any of the sequencers' states have been updated this cycle
	bool sequencers_updated = (clock_tick || events->reset);

	// Bitflags storing which output channels will fire this cycle, indexed by
	// `OutputChannel`.
	uint8_t out_channels_firing = euclid_update_sequencers(events);

	/* OUTPUT */

	for (uint8_t out_channel = 0; out_channel < OUTPUT_NUM_CHANNELS; out_channel++) {
		bool should_fire = out_channels_firing & (0x01 << out_channel);
		if (should_fire) {
			output_set_high((OutputChannel)out_channel);
		}
	}

	if (sequencers_updated) {
		// Update output pulse length and timeout
		Milliseconds time_since_last = now - output_pulse_timeout.inner.start;
		Milliseconds pulse_length = CONSTRAIN(time_since_last / 5, 2, 5);
		output_pulse_timeout.inner.duration = pulse_length;

		timeout_once_reset(&output_pulse_timeout, now);
	}

	// FINISH ANY PULSES THAT ARE ACTIVE
	if (timeout_once_fired(&output_pulse_timeout, now)) {
		output_clear_all();
	}

	/* DRAWING - ACTIVE CHANNEL DISPLAY */

	if (events->enc_push != ENCODER_NONE) {
		active_channel_display_draw(active_channel);
	}

	/* DRAWING - CHANNELS */

	if (sequencers_updated) {
		// Update playhead flash duration based on the last interval between two
		// clock or reset signals received.
		Milliseconds previous_period = now - last_clock_or_reset;
		playhead_flash_timeout.inner.duration = calc_playhead_flash_time(previous_period);
		last_clock_or_reset = now;

		// Reset playhead flash
		timeout_once_reset(&playhead_flash_timeout, now);

		// Reset playhead idle
		timeout_reset(&playhead_idle_timeout, now);
	}

	// Update playhead idle - Make playhead flash periodically when it hasn't
	// moved in a certain amount of time
	bool playhead_flash_updated = false;
	if (timeout_fired(&playhead_idle_timeout, now)) {
		if (timeout_loop(&playhead_idle_loop_timeout, now)) {
			playhead_flash_timeout.inner.duration = PLAYHEAD_FLASH_TIME_DEFAULT;
			timeout_once_reset(&playhead_flash_timeout, now);
			playhead_flash_updated = true;
		}
	}

	// Update playhead flash
	if (timeout_once_fired(&playhead_flash_timeout, now)) {
		playhead_flash_updated = true;
	}

	// Tracks if the screen needs to be redrawn.
	bool needs_redraw = sequencers_updated || playhead_flash_updated;

	if (param_knob_moved.valid) {
		// If parameters have changed, reset the adjustment display timeout and state
		adjustment_display_state.channel = active_channel;
		adjustment_display_state.parameter = param_knob_moved.inner;
		adjustment_display_state.visible = true;
		timeout_reset(&adjustment_display_timeout, now);

		needs_redraw = true;
	} else {
		// If no parameters have changed, check if the adjustment display still
		// needs to be shown, and hide it if it doesn't
		if (adjustment_display_state.visible) {
			bool should_be_hidden = timeout_fired(&adjustment_display_timeout, now);
			if (should_be_hidden) {
				adjustment_display_state.visible = false;
				needs_redraw = true;
			}
		}
	}

	if (needs_redraw) {
		euclid_draw_channels();
	}

	/* DRAWING - OUTPUT INDICATORS */

	if (sequencers_updated) {
		indicators_output_draw_latching(out_channels_firing);
	}
}

/* INTERNAL */

static void euclid_handle_encoder_push(EncoderIdx enc_idx) {
	ChannelOpt active_channel_new = channel_for_encoder(enc_idx);
	if (active_channel_new.valid) {
		euclid_state.active_channel = active_channel_new.inner;
	}
}

static EuclidParamOpt euclid_handle_encoder_move(const int16_t *enc_move) {
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

static uint8_t euclid_update_sequencers(const InputEvents *events) {
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

static void euclid_draw_channels(void) {
	for (uint8_t channel = 0; channel < NUM_CHANNELS; channel++) {
		draw_channel((Channel)channel);
	}
}

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

static Milliseconds calc_playhead_flash_time(Milliseconds clock_period) {
	// This is a standard "scale from input range to output range" function, but
	// it uses specific ranges so that we can avoid multiplication or division by
	// numbers that aren't powers of 2.

	// 256ms min period = ~234bpm
	// 1280ms max period = ~47bpm
	// 1280-256 = an input range of 1024, or 2^10
	clock_period = CONSTRAIN(clock_period, 256, 1280);
	// Subtract input min
	Milliseconds delta = clock_period - 256;
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
	ParamIdx idx = euclid_param_idx(channel, kind);
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
