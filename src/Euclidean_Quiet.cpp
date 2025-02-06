#include "config.h"

#if EEPROM_READ || EEPROM_WRITE
#include <EEPROM.h>
#endif

#include <Arduino.h>

#include "common/timeout.h"
#include "common/types.h"
#include "hardware/input.h"
#include "hardware/led.h"
#include "hardware/output.h"
#include "hardware/properties.h"
#include "modes/clock.h"
#include "modes/euclid.h"
#include "ui/active_channel.h"
#include "ui/framebuffer.h"
#include "ui/framebuffer_led.h"
#include "ui/indicators.h"
#include "ui/led_sleep.h"

#include <euclidean.h>

// clang-format off
/* 
  Alternate "Quiet" Firmware for Sebsongs Modular Euclidean Eurorack module.

  Changes from official v1.2 firmware:
  - Behavior changes:
    - Channel 1 is now selected when the module starts up.
    - The internal clock no longer starts when the module starts up.
    - Patterns generated are now accurate to the original Euclidean Rhythms paper.
    - LED sleep timeout now takes into account encoder manipulations.
    - LED now dims itself before sleeping.
		- Removed LED blink sequence on back of module when starting up.
  - UI Polish:
    - All steps of a generated pattern are now visible at all times - no more sequencer pages.
    - The steps of a pattern are visible while adjusting its length.
    - New adjustment display when setting pattern length.
    - The effects of resetting the sequencers is now immediately visible.
    - There is now an indicator LED for Reset input, next to the one labeled "Trig".
    - Output indicator LEDs now stay lit for the entire duration of the step.
    - The "Trig" LED indicator now illuminates every clock pulse instead of alternating ones.
    - Made channel selection easier to see (two dots instead of 4 overlapping).
		- Removed LED sleep animations.
    - Before a clock trigger has been received, each channel's pattern is displayed.
    - The adjustment display now hides itself after a certain amount of time, instead of waiting for the next clock signal.
  - Bugs Fixed:
    - The internal clock started up again when the reset button was pressed.
    - Sometimes ignored "Reset" input that happened simultaneously with "Trig" input.
    - Turning the density up if it was already at the maximum would cause it to toggle between the two highest values.
    - When the LED matrix wakes from sleep mode, the channel selection would not be displayed
    - Reset did not function for any channel if channel 1's playhead was at position 0.
    - Validating faulty saved data did not happen until after that data was used.
    - When reducing the length parameter for a channel, its adjusted density would not be saved.
    - When a pattern length was reduced to below the sequencer's current position, the position was not reset.
  - Development:
    - Basically a complete rewrite of the firmware.
    - Migrated project to PlatformIO from Arduino IDE.
    - Source code is now formatted by clang-format.
    - Added automated tests for Euclidean rhythm generation algorithm.
*/

/* Sebsongs Modular Euclidean v. 1.2. Dec 2 2022.
 *  
 * Revision notes:
 * - Density can now be zero at all lengths.
 * - Trigger lengths are now around 10ms on all channels.
 * - Wake up from sleep with reset switch now available.
 * - Blink pattern for revision check added. LED 13 blinks 4 times (50ms ON / 200ms OFF).
 * - Renamed internal sleep function to "zleep" due to conflicts with Arduino Nano Every.
 * - Fixed bug where offset could go out of range and never resolve.
 * - Fixed bug where sequencer freezed when encoder switches were pressed and hold.
 */


/* Old notes from Tom Whitwell and others.
 
  Instructions:

  -- Note: when first turned on, it runs automatically from a fixed speed internal clock until it receives
  a pulse at the clock input, after that it only triggers externally --

  When the rhythms are playing, the display flips between page 1 (steps 1-8) and page 2 (steps 9-16).

  Here's what the display shows...

  Row 1: What step is Output 1 playing
  Row 2: Output 1 Pattern (steps 1-8 or 9-16)

  Row 3: What step is Output 2 playing
  Row 4: Output 2 Pattern (steps 1-8 or 9-16)

  Row 5: What step is Output 3 playing
  Row 6: Output 3 Pattern (steps 1-8 or 9-16)

  Row 7: Which channel is selected

  - 2 dots on the left for Channel 1
  - 2 dots in the middle for Channel 2
  - 2 dots on the right for Channel 3

  Row 8: Current triggers

  1. Input trigger
  2. -
  3. Output 1 trigger
  4. Output 1 off-beat trigger (when Output 1 isn't playing
  5. Output 2 trigger
  6. -
  7. Output 3 trigger
  8. -

  - When you have Channel 1 selected and you rotate the N- and K- and Offset knobs,
  Rows 1 and 2 will respectively show the pattern length (N), pattern density (K) or Offset (O).

  The same goes for Channel 2 (Rows 3 and 4) and Channel 3 (Rows 5 and 6)

  - Rotating the Offset encoder clockwise rotates the steps up to one full rotation

  Example:

  X 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 - Original 16-step pattern (N = 16, K = 1)
  0 X 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 - Offset of 1
  0 0 X 0 0 0 0 0 0 0 0 0 0 0 0 0 0 - Offset of 2
  0 0 0 X 0 0 0 0 0 0 0 0 0 0 0 0 0 - Offset of 3
  0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 X - Offset of 15
  X 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 - Offset of 16

  >>> Sneak-Thief's design notes <<<

  - Added Encoder library to stabilize encoder use
  - Cleaned up code a bit
  - Created schematic with input/output protection: http://sneak-thief.com/modular/tombolas-euclidean-0.7.png
  - Added reset gate input
  - Added Offset control
  - Added 10K pulldown resistor to switch outputs
  - Changed 15K resistors to 10K
  - Added more error checking for eeprom reads / values to reduce risk of crashes

  >>> Syinsi/THX2112's design notes <<<

  - Updated schematic and included PCB files. Check distribution folder for files.
  - Uses separate MAX7219 IC and 8x8 matrix's.
  - Transistor-driven outputs.
  - BAT54S dual-schottky input protection.
  - Isolated 1.5A 5V supply for led matrix.

  To do
  - Add CV control of N, K and Offset using analog pins 5,6,7
  - OR Add gate control of two pairs of N, K or Offset using analog pins 4,5,6,7

  >>> Tombola's original design notes <<<

  To do
  - Error checking for eeprom reads / values to reduce risk of crashes
  - Find cause of ocassional skipped beats?

  Done
  - Connect 'off beat' for channel 1 to the spare output
  - something causing channel 1 to stick - N changes don't appear
  - When N is turned down, reduce K accordingly
  - make tick pulse correctly
  - Fix drawing of beats  as beats are playing
  - binary display of K & N not right
  - remove serial print / debug routine
  - remove delay - replace with 'all pulses off after 5ms routine
  - Fix crashing issue with low n
  - channels 1 and 3 require tweak to start running
  - Display OK
  - Pulse in working up to audio rates
  - Got flashing working - system is outputting euclidean beats!
  - Integrate encoders & eeprom
  - Add encoder display - light up row as it's turning DONE
  - 3-way switch working
  - Active channel indicator
  - Sleep / wake system & animation
  - DONE Remove serial printing - slows down?

  TO DO:
  - Implement offset (added by Sneak-Thief)
  - Add reset outputs and input and button
  - Add CV control of N and K (and later Offset) using analog pins 5,6,7

  BUGS:
  - FIXED Still display issues - 1111 on the end of beat displays - seems not to be clearing properly
  - FIXED Display on beats where k=n - seems to show loads of binary 111 on the end
  - FIXED Fix the bottom row - currently output flashers reversed

  Hardware notes:

  Display:
  din = d2
  clk = d3
  load = d4

  Encoders:
  Encoder 1a - k / beats = d5
  Encoder 1b - k / beats = d6
  Encoder 2a - n / length = d7
  Encoder 2b - n / length  = d8
  Encoder 3a - Offset = d9
  Encoder 3 b - offset = d10

  Pulse outputs:
  1 = d11
  2 = d12
  3 = d13

  Switches / misc :
  reset switch = A1
  pulse input = A0
  spare jack out = A3
  encoder switch = A2  - 3 buttons share one analog input:

  10k resistor ladder around 3 push button switches
  +5v -- 10k -- switch -- 10k -- switch -- 10k -- switch -- 10k -- GND
  Other ends of switches going to Analog in 0 and also 10k to GND

  Order: [HIGHEST R] Offset, Length, Density, (unpressed) [LOWEST R]

*/
// clang-format on

/* GLOBALS */

static Milliseconds time;
static Milliseconds last_clock_or_reset;

static Channel active_channel; // Channel that is currently active
static TimeoutOnce output_pulse_timeout = {
    .inner = {.duration = 5}}; // Pulse length, set based on the time since last trigger

// Track the time since the playhead has moved so we can make it flash in its idle loop
static Timeout playhead_idle_timeout = {.duration = PLAYHEAD_IDLE_TIME};
// Loop for making the playhead flash periodically after it is idle
static Timeout playhead_idle_loop_timeout = {.duration = PLAYHEAD_IDLE_LOOP_PERIOD};

static Timeout adjustment_display_timeout = {.duration = ADJUSTMENT_DISPLAY_TIME};

typedef struct EuclideanChannelUpdate {
	uint8_t length;
	uint8_t density;
	uint8_t offset;
	bool length_changed;
	bool density_changed;
	bool offset_changed;
} EuclideanChannelUpdate;

static const EuclideanChannelUpdate EUCLIDEAN_UPDATE_EMPTY = {
    .length = 0,
    .density = 0,
    .offset = 0,
    .length_changed = false,
    .density_changed = false,
    .offset_changed = false,
};

#if LOGGING_ENABLED && LOGGING_CYCLE_TIME
Microseconds cycle_time_max;
static Timeout log_cycle_time_timeout = {.duration = LOGGING_CYCLE_TIME_INTERVAL};
#endif

/* DECLARATIONS */

static void init_serial(void);
static ChannelOpt channel_for_encoder(EncoderIdx enc_idx);
static Milliseconds calc_playhead_flash_time(Milliseconds clock_period);
/// Load state from EEPROM into the given `EuclideanState`
static void eeprom_load(EuclideanState *s);
static inline int eeprom_addr_length(Channel channel);
static inline int eeprom_addr_density(Channel channel);
static inline int eeprom_addr_offset(Channel channel);
#if LOGGING_ENABLED && LOGGING_INPUT
static void log_input_events(const InputEvents *events);
#endif

/* MAIN */

void setup() {
	time = millis();

	led_init();
	led_sleep_init(time);
	eeprom_load(&euclidean_state);
	euclid_validate_state(&euclidean_state);
	init_serial();
	input_init();
	output_init();

	// Initialise generated rhythms
	for (int a = 0; a < NUM_CHANNELS; a++) {
		generated_rhythms[a] =
		    euclidean_pattern_rotate(euclidean_state.channels[a].length, euclidean_state.channels[a].density,
		                             euclidean_state.channels[a].offset);
	}

	led_wake();

	// Select first channel on startup
	active_channel = CHANNEL_1;

	// Draw initial UI
	euclid_draw_channels();
	active_channel_display_draw(active_channel);
}

void loop() {
	time = millis();

#if LOGGING_ENABLED && LOGGING_CYCLE_TIME
	Microseconds cycle_time_start = micros();
#endif

	/* INPUT EVENTS */

	InputEvents events_in = INPUT_EVENTS_EMPTY;
	input_update(&events_in, time);
#if LOGGING_ENABLED && LOGGING_INPUT
	log_input_events(&events_in);
#endif

	/* HANDLE INPUT */

	// Handle Encoder Pushes
	ChannelOpt active_channel_new = channel_for_encoder(events_in.enc_push);
	if (active_channel_new.valid) {
		active_channel = active_channel_new.inner;
	}

	EuclideanParamOpt knob_moved_for_param = EUCLIDEAN_PARAM_OPT_NONE;
#if EEPROM_WRITE
	EuclideanChannelUpdate params_update = EUCLIDEAN_UPDATE_EMPTY;
#endif

	// Handle Length Knob Movement
	int nknob = events_in.enc_move[ENCODER_1];
	if (nknob != 0) {
		knob_moved_for_param = euclidean_param_opt(EUCLIDEAN_PARAM_LENGTH);

		Channel channel = active_channel;
		EuclideanChannelState channel_state = euclidean_state.channels[channel];
		int length = channel_state.length;
		uint8_t density = channel_state.density;
		uint8_t offset = channel_state.offset;
		uint8_t position = channel_state.position;

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
			euclidean_state.channels[channel].density = density;

#if EEPROM_WRITE
			params_update.density = density;
			params_update.density_changed = true;
#endif
		}
		if ((offset >= (length + nknob)) && (offset < 16)) {
			offset += nknob;
			euclidean_state.channels[channel].offset = offset;

#if EEPROM_WRITE
			params_update.offset = offset;
			params_update.offset_changed = true;
#endif
		}

		length += nknob;
		euclidean_state.channels[channel].length = length;

		// Reset position if length has been reduced past it
		if (position >= length) {
			euclidean_state.channels[channel].position = 0;
		}

#if EEPROM_WRITE
		params_update.length = length;
		params_update.length_changed = true;
#endif
	}

	// Handle Density Knob Movement
	int kknob = events_in.enc_move[ENCODER_2];
	if (kknob != 0) {
		knob_moved_for_param = euclidean_param_opt(EUCLIDEAN_PARAM_DENSITY);

		Channel channel = active_channel;
		EuclideanChannelState channel_state = euclidean_state.channels[channel];
		int length = channel_state.length;
		int density = channel_state.density;

		// Keep density in bounds
		if (density + kknob > length) {
			kknob = 0;
		}
		if (density + kknob < BEAT_DENSITY_MIN) {
			kknob = 0;
		}

		density += kknob;
		euclidean_state.channels[channel].density = density;

#if EEPROM_WRITE
		params_update.density = density;
		params_update.density_changed = true;
#endif
	}

	// Handle Offset Knob Movement
	int oknob = events_in.enc_move[ENCODER_3];
	if (oknob != 0) {
		knob_moved_for_param = euclidean_param_opt(EUCLIDEAN_PARAM_OFFSET);

		Channel channel = active_channel;
		EuclideanChannelState channel_state = euclidean_state.channels[channel];
		int length = channel_state.length;
		int offset = channel_state.offset;

		// Keep offset in bounds
		if (offset + oknob > length - 1) {
			oknob = 0;
		}
		if (offset + oknob < BEAT_OFFSET_MIN) {
			oknob = 0;
		}

		offset += oknob;
		euclidean_state.channels[channel].offset = offset;

#if EEPROM_WRITE
		params_update.offset = offset;
		params_update.offset_changed = true;
#endif
	}

	// Update Generated Rhythms Based On Parameter Changes
	if (knob_moved_for_param.valid) {
		Channel channel = active_channel;
		EuclideanChannelState channel_state = euclidean_state.channels[channel];
		uint8_t length = channel_state.length;
		uint8_t density = channel_state.density;
		uint8_t offset = channel_state.offset;

		generated_rhythms[channel] = euclidean_pattern_rotate(length, density, offset);

#if LOGGING_ENABLED
		if (knob_moved_for_param.inner == EUCLIDEAN_PARAM_LENGTH) {
			Serial.print("length: ");
			Serial.println(length);
		} else if (knob_moved_for_param.inner == EUCLIDEAN_PARAM_DENSITY) {
			Serial.print("density: ");
			Serial.println(density);
		} else {
			Serial.print("offset: ");
			Serial.println(offset);
		}
#endif
	}

	/* UPDATE INTERNAL CLOCK */

	internal_clock_update(&events_in, time);

	/* UPDATE SEQUENCER */

	// Clock ticks merge the internal and external clocks
	bool clock_tick = events_in.trig || events_in.internal_clock_tick;

	// Tracks if any of the sequencers' states have been updated this cycle
	bool sequencers_updated = (clock_tick || events_in.reset);

	// Bitflags storing which output channels will fire this cycle, indexed by
	// `OutputChannel`.
	uint8_t out_channels_firing = euclid_update(&events_in);

	/* OUTPUT */

	for (uint8_t out_channel = 0; out_channel < OUTPUT_NUM_CHANNELS; out_channel++) {
		bool should_fire = out_channels_firing & (0x01 << out_channel);
		if (should_fire) {
			output_set_high((OutputChannel)out_channel);
		}
	}

	if (sequencers_updated) {
		// Update output pulse length and timeout
		Milliseconds pulse_length = constrain(((time - output_pulse_timeout.inner.start) / 5), 2, 5);
		output_pulse_timeout.inner.duration = pulse_length;

		timeout_once_reset(&output_pulse_timeout, time);
	}

	// FINISH ANY PULSES THAT ARE ACTIVE
	if (timeout_once_fired(&output_pulse_timeout, time)) {
		output_clear_all();
	}

	/* DRAWING - INDICATORS */

	indicators_input_update(&events_in, time);

	if (sequencers_updated) {
		indicators_output_draw_latching(out_channels_firing);
	}

	/* DRAWING - ACTIVE CHANNEL DISPLAY */

	if (events_in.enc_push != ENCODER_NONE) {
		active_channel_display_draw(active_channel);
	}

	/* DRAWING - CHANNELS */

	if (sequencers_updated) {
		// Update playhead flash duration based on the last interval between two
		// clock or reset signals received.
		Milliseconds previous_period = time - last_clock_or_reset;
		playhead_flash_timeout.inner.duration = calc_playhead_flash_time(previous_period);
		last_clock_or_reset = time;

		// Reset playhead flash
		timeout_once_reset(&playhead_flash_timeout, time);

		// Reset playhead idle
		timeout_reset(&playhead_idle_timeout, time);
	}

	// Update playhead idle - Make playhead flash periodically when it hasn't
	// moved in a certain amount of time
	bool playhead_flash_updated = false;
	if (timeout_fired(&playhead_idle_timeout, time)) {
		if (timeout_loop(&playhead_idle_loop_timeout, time)) {
			playhead_flash_timeout.inner.duration = PLAYHEAD_FLASH_TIME_DEFAULT;
			timeout_once_reset(&playhead_flash_timeout, time);
			playhead_flash_updated = true;
		}
	}

	// Update playhead flash
	if (timeout_once_fired(&playhead_flash_timeout, time)) {
		playhead_flash_updated = true;
	}

	// Tracks if the screen needs to be redrawn.
	bool needs_redraw = sequencers_updated || playhead_flash_updated;

	if (knob_moved_for_param.valid) {
		// If parameters have changed, reset the adjustment display timeout and state
		adjustment_display_state.channel = active_channel;
		adjustment_display_state.parameter = knob_moved_for_param.inner;
		adjustment_display_state.visible = true;
		timeout_reset(&adjustment_display_timeout, time);

		needs_redraw = true;
	} else {
		// If no parameters have changed, check if the adjustment display still
		// needs to be shown, and hide it if it doesn't
		if (adjustment_display_state.visible) {
			bool should_be_hidden = timeout_fired(&adjustment_display_timeout, time);
			if (should_be_hidden) {
				adjustment_display_state.visible = false;
				needs_redraw = true;
			}
		}
	}

	if (needs_redraw) {
		euclid_draw_channels();
	}

	/* UPDATE LED DISPLAY */

	framebuffer_update_color_animations(time);
	framebuffer_copy_row_to_display();

	/* UPDATE LED SLEEP */

	bool postpone_sleep = input_events_contains_any_external(&events_in);
	LedSleepUpdate sleep_update = led_sleep_update(postpone_sleep, time);
	if (sleep_update == LED_SLEEP_UPDATE_WAKE) {
		led_wake();
	} else if (sleep_update == LED_SLEEP_UPDATE_DIM) {
		led_dim();
	} else if (sleep_update == LED_SLEEP_UPDATE_SLEEP) {
		led_sleep();
	}

	/* EEPROM WRITES */

#if EEPROM_WRITE
	if (params_update.length_changed) {
		EEPROM.update(eeprom_addr_length(active_channel), params_update.length);
	}
	if (params_update.density_changed) {
		EEPROM.update(eeprom_addr_density(active_channel), params_update.density);
	}
	if (params_update.offset_changed) {
		EEPROM.update(eeprom_addr_offset(active_channel), params_update.offset);
	}
#endif

#if LOGGING_ENABLED && LOGGING_EEPROM && EEPROM_WRITE
	if (params_update.length_changed) {
		Serial.print("EEPROM Write: Length= ");
		Serial.print(eeprom_addr_length(active_channel));
		Serial.print(" ");
		Serial.println(params_update.length);
	}
	if (params_update.density_changed) {
		Serial.print("EEPROM Write: Density= ");
		Serial.print(eeprom_addr_density(active_channel));
		Serial.print(" ");
		Serial.println(params_update.density);
	}
	if (params_update.offset_changed) {
		Serial.print("EEPROM Write: Offset= ");
		Serial.print(eeprom_addr_offset(active_channel));
		Serial.print(" ");
		Serial.println(params_update.offset);
	}
#endif

#if LOGGING_ENABLED && LOGGING_CYCLE_TIME
	Microseconds cycle_time = micros() - cycle_time_start;
	if (cycle_time > cycle_time_max) {
		cycle_time_max = cycle_time;
	}

	if (timeout_loop(&log_cycle_time_timeout, time)) {
		Serial.print("Max Cycle Time: ");
		Serial.println(cycle_time_max);
		cycle_time_max = 0;
	}

#endif
}

/* INTERNAL */

static void init_serial(void) {
#if LOGGING_ENABLED
	Serial.begin(9600);
#endif
}

static ChannelOpt channel_for_encoder(EncoderIdx enc_idx) {
	switch (enc_idx) {
		case ENCODER_1:
			return {.inner = CHANNEL_2, .valid = true};
			break;
		case ENCODER_2:
			return {.inner = CHANNEL_3, .valid = true};
			break;
		case ENCODER_3:
			return {.inner = CHANNEL_1, .valid = true};
			break;
		default:
			return CHANNEL_OPT_NONE;
			break;
	}
}

static Milliseconds calc_playhead_flash_time(Milliseconds clock_period) {
	// This is a standard "scale from input range to output range" function, but 
	// it uses specific ranges so that we can avoid multiplication or division by
	// numbers that aren't powers of 2.

	// 256ms min period = ~234bpm
	// 1280ms max period = ~47bpm
	// 1280-256 = an input range of 1024, or 2^10
	clock_period = constrain(clock_period, 256, 1280);
	// Subtract input min
	Milliseconds delta = clock_period - 256;
	// (delta / input range) * output range. Input range is 2^10, output range is
	// 2^7, so just divide by 2^3.
	Milliseconds result = delta >> 3;
	// Add output min
	result += 64;
	return result;
}

static void eeprom_load(EuclideanState *s) {
	/*
	EEPROM Schema:
	Channel 1: length = 1 density = 2 offset = 7
	Channel 2: length = 3 density = 4 offset = 8
	Channel 3: length = 5 density = 6 offset = 9
	*/

#if EEPROM_READ
	for (uint8_t c = 0; c < NUM_CHANNELS; c++) {
		Channel channel = (Channel)c;
		s->channels[c].length = EEPROM.read(eeprom_addr_length(channel));
		s->channels[c].density = EEPROM.read(eeprom_addr_density(channel));
		s->channels[c].offset = EEPROM.read(eeprom_addr_offset(channel));
		s->channels[c].position = 0;
	}
#endif
}

static inline int eeprom_addr_length(Channel channel) { return (channel * 2) + 1; }

static inline int eeprom_addr_density(Channel channel) { return (channel * 2) + 2; }

static inline int eeprom_addr_offset(Channel channel) { return channel + 7; }

#if LOGGING_ENABLED && LOGGING_INPUT
static void log_input_events(const InputEvents *events) {
	if (events->reset) {
		Serial.println("INPUT: Reset");
	}
	if (events->trig) {
		Serial.println("INPUT: Trigger");
	}
	if (events->enc_move[ENCODER_1] != 0) {
		Serial.print("ENC_1: Move ");
		Serial.println(events->enc_move[ENCODER_1]);
	}
	if (events->enc_move[ENCODER_2] != 0) {
		Serial.print("ENC_2: Move ");
		Serial.println(events->enc_move[ENCODER_2]);
	}
	if (events->enc_move[ENCODER_3] != 0) {
		Serial.print("ENC_3: Move ");
		Serial.println(events->enc_move[ENCODER_3]);
	}
}
#endif
