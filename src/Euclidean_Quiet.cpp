extern "C" {
#include "config.h"
}

#if EEPROM_READ || EEPROM_WRITE
#include <EEPROM.h>
#endif
#include <Encoder.h>
#include <LedControl.h>

extern "C" {
#include <euclidean.h>
#include "hardware.h"
#include "output.h"
#include "timeout.h"
#include "types.h"
}

/* 
  Alternate "Quiet" Firmware for Sebsongs Modular Euclidean Eurorack module.

  Changes from official v1.2 firmware:
  - Behavior changes:
    - Channel 1 is now selected when the module starts up
    - The internal clock no longer starts when the module starts up.
    - Patterns generated are now accurate to the original Euclidean Rhythms paper.
    - LED sleep timeout now takes into account encoder manipulations
  - UI Polish:
    - All steps of a generated pattern are now visible at all times - no more sequencer pages.
    - The steps of a pattern are visible while adjusting its length.
    - The effects of resetting the sequencers is now immediately visible.
    - There is now an indicator LED for Reset input, next to the one labeled "Trig".
    - Output indicator LEDs now stay lit for the entire duration of the step.
    - The "Trig" LED indicator now illuminates every clock pulse instead of alternating ones.
    - Made channel selection easier to see (two dots instead of 4 overlapping).
    - Shortened the time that LEDs stay lit when setting pattern length for a channel.
    - Before a clock trigger has been received, the full pattern is drawn for each channel.
    - The adjustment display now hides itself after a certain amount of time, instead of waiting for the next clock signal.
  - Bugs Fixed:
    - The internal clock started up again when the reset button was pressed.
    - Sometimes ignored "Reset" input that happened simultaneously with "Trig" input
    - Turning the density up if it was already at the maximum would cause it to toggle between the two highest values.
    - When the LED matrix wakes from sleep mode, the channel selection would not be displayed
    - Reset did not function for any channel if channel 1's playhead was at position 0.
    - Validating faulty saved data did not happen until after that data was used.
    - When reducing the length parameter for a channel, its adjusted density would not be saved.
    - When a pattern length was reduced to below the sequencer's current position, the position was not reset
  - Development:
    - Migrated firmware project to PlatformIO.
    - Added tests for Euclidean rhythm generation.
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

/* SOFTWARE CONSTANTS */

#define NUM_CHANNELS 3
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

/// Row of LED Display marked "CH SEL" on panel
#define LED_CH_SEL_Y 6
/// Column of LED Display marked "TRIG" on panel
#define LED_IN_TRIG_X 0
/// Column of LED Display where Reset input is indicated
#define LED_IN_RESET_X 1
/// Column of LED Display marked "1" on panel
#define LED_OUT_CH1_X 2
/// Column of LED Display marked "OFF" on panel
#define LED_OUT_OFFBEAT_X 3
/// Column of LED Display marked "2" on panel
#define LED_OUT_CH2_X 5
/// Column of LED Display marked "3" on panel
#define LED_OUT_CH3_X 7
/// Row of LED Display marked "OUT" on panel
#define LED_OUT_Y 7

/* GLOBALS */

bool internal_clock_enabled = INTERNAL_CLOCK_DEFAULT;

// Initialize objects for reading encoders
// (from the Encoder.h library)
Encoder Enc1(PIN_ENC_2B, PIN_ENC_2A); // Length  / N
Encoder Enc2(PIN_ENC_1B, PIN_ENC_1A); // Density / K
Encoder Enc3(PIN_ENC_3B, PIN_ENC_3A); // Offset  / O

// Initialize objects for controlling LED matrix
// (from LedControl.h library)
// 1 is maximum number of devices that can be controlled
LedControl lc = LedControl(PIN_OUT_LED_DATA, PIN_OUT_LED_CLOCK, PIN_OUT_LED_SELECT, 1);

/// Represents a method of deciding on illumination of a pixel on the LED matrix
/// display.
typedef enum Color {
  /// Do not light up this pixel
  COLOR_OFF = 0,
  /// Light up this pixel
  COLOR_ON = 1,
  /// Blink every other LED rapidly for this pixel
  COLOR_DAZZLE = 2,
  /// Show a fast marching ants pattern for this pixel
  COLOR_ANTS = 3
} Color;

#define ANIM_DAZZLE_NUM_FRAMES 2
Timeout anim_dazzle_timeout = { .duration = ANIM_DAZZLE_INTERVAL };
uint8_t anim_dazzle_frame = 0;

#define ANIM_ANTS_NUM_FRAMES 4
Timeout anim_ants_timeout = { .duration = ANIM_ANTS_INTERVAL };
uint8_t anim_ants_frame = 0;

/// Buffer that can be drawn into and manipulated before being drawn to the
/// hardware display. 2 bits per pixel, so it supports 4 colors.
uint16_t framebuffer[LED_ROWS];

/// To keep latency from spiking, we only draw one row of the framebuffer to the
/// LED matrix at a time. The row that gets drawn rotates between the 8 rows of 
/// the framebuffer to keep visual latency equal for all rows. 
uint8_t framebuffer_out_row;

/// References one of the three channels
typedef enum Channel {
  CHANNEL_1,
  CHANNEL_2,
  CHANNEL_3,
} Channel;

/// A parameter of the Euclidean rhythm generator
enum EuclideanParam {
  EUCLIDEAN_PARAM_NONE,
  EUCLIDEAN_PARAM_LENGTH,
  EUCLIDEAN_PARAM_DENSITY,
  EUCLIDEAN_PARAM_OFFSET,
};

/// State of the Euclidean rhythm generator and sequencer for a single channel
typedef struct EuclideanChannelState {
  /// Number of steps in the Euclidean rhythm, 1-16
  uint8_t length;
  /// Number of active steps in the Euclidean rhythm, 1-16
  uint8_t density;
  /// Number of steps to rotate the Euclidean rhythm to the right, 1-16
  uint8_t offset;
  /// Step index representing the playhead position for this channel's sequencer, 0-15
  uint8_t position; 
} EuclideanChannelState;

/// State of the entire Euclidean module
typedef struct EuclideanState {
  /// State for each of this module's channels, indexed by `Channel` enum.
  EuclideanChannelState channels[NUM_CHANNELS];
  bool sequencer_running;
} EuclideanState;

static EuclideanState euclidean_state = {
  .channels = {
    { .length = BEAT_LENGTH_DEFAULT, .density = BEAT_DENSITY_DEFAULT, .offset = BEAT_OFFSET_DEFAULT, .position = 0 },
    { .length = BEAT_LENGTH_DEFAULT, .density = BEAT_DENSITY_DEFAULT, .offset = BEAT_OFFSET_DEFAULT, .position = 0 },
    { .length = BEAT_LENGTH_DEFAULT, .density = BEAT_DENSITY_DEFAULT, .offset = BEAT_OFFSET_DEFAULT, .position = 0 }
  },
  .sequencer_running = false,
};

Milliseconds time;
static Milliseconds last_clock_or_reset;

/// Stores each generated Euclidean rhythm as 16 bits. Indexed by channel number.
uint16_t generated_rhythms[NUM_CHANNELS];
Channel active_channel; // Channel that is currently active
static Timeout internal_clock_timeout = { .duration = INTERNAL_CLOCK_PERIOD };
static Timeout output_pulse_timeout = { .duration = 5 }; // Pulse length, set based on the time since last trigger

static TimeoutOnce trig_indicator_timeout = { .inner = {.duration = INPUT_INDICATOR_FLASH_TIME} };
static TimeoutOnce reset_indicator_timeout = { .inner = {.duration = INPUT_INDICATOR_FLASH_TIME} };
/// Stores which output channels have active steps this step of their sequencer,
/// as bitflags indexted by `OutputChannel`.
uint8_t output_channels_active_step_bitflags = 0;
static TimeoutOnce output_indicator_blink_timeout = { .inner = { .duration = OUTPUT_INDICATOR_BLINK_TIME } };

// Tracks the playhead blink itself
static TimeoutOnce playhead_blink_timeout = { .inner = { .duration = PLAYHEAD_BLINK_TIME_DEFAULT } };
#if PLAYHEAD_IDLE
// Track the time since the playhead has moved so we can make it blink in its idle loop
static Timeout playhead_idle_timeout = { .duration = PLAYHEAD_IDLE_TIME };
// Loop for making the playhead blink periodically after it is idle
static Timeout playhead_idle_loop_timeout = { .duration = PLAYHEAD_IDLE_LOOP_PERIOD };
#endif

/// For recognizing trigger in rising edges
int trig_in_value_previous = 0; 
bool reset_active = false;
unsigned long channelPressedCounter = 0;
static Timeout encoder_read_timeout = { .duration = READ_DELAY };

typedef struct AdjustmentDisplayState {
  /// Which channel is currently showing its adjustment display. Only one 
  /// adjustment display can be visible at a time.
  Channel channel;
  /// The parameter that is being displayed in the adjustment display.
  EuclideanParam parameter;
  /// Is the adjustment display showing currently
  bool visible;
} AdjustmentDisplayState;
static AdjustmentDisplayState adjustment_display_state = {
  .channel = CHANNEL_1,
  .parameter = EUCLIDEAN_PARAM_NONE,
  .visible = false,
};
static Timeout adjustment_display_timeout = { .duration = ADJUSTMENT_DISPLAY_TIME };

bool led_sleep_mode_active = false;
static Timeout led_sleep_timeout = { .duration = LED_SLEEP_TIME };

/// Represents the three encoders in the `InputEvents` struct.
/// Indexes into its arrays.
enum EncoderIdx {
  ENCODER_1 = 0,
  ENCODER_2 = 1,
  ENCODER_3 = 2,
  ENCODER_NONE = 4,
};

/// Record of any input events that were received this cycle
typedef struct InputEvents {
  /// Some encoders were rotated, indexed by `EncoderIdx`
  int16_t enc_move[NUM_CHANNELS];
  /// An encoder was pushed
  EncoderIdx enc_push;
  /// "Trig" input detected a rising edge
  bool trig;
  /// "Reset" input or button detected a rising edge
  bool reset;
  /// The internal clock generated a tick
  bool internal_clock_tick;
} InputEvents;

/// An instance of `InputEvents` which represents no events happening
static const InputEvents INPUT_EVENTS_EMPTY = {
  .enc_move = { 0, 0, 0 },
  .enc_push = ENCODER_NONE,
  .trig = false,
  .reset = false,
  .internal_clock_tick = false,
};

#if LOGGING_ENABLED && LOGGING_CYCLE_TIME
Microseconds cycle_time_max;
static Timeout log_cycle_time_timeout = { .duration = LOGGING_CYCLE_TIME_INTERVAL };
#endif

/* INTERNAL */

/// Returns true if `events` contains any externally-generated events
static bool input_events_contains_any_external(InputEvents *events);
static Milliseconds calc_playhead_blink_time(Milliseconds clock_period);
static void sequencer_handle_reset();
static void sequencer_handle_clock();
static void sequencer_advance();
/// What is the output that should be sent for each sequencer this cycle
/// @return Bitflags, indexed using `OutputChannel`. 1 = begin an output pulse this cycle for this channel, 0 = do nothing for this channel 
static uint8_t sequencer_read_current_step();
static void draw_channels();
static inline void draw_channel(Channel channel);
static inline void draw_channel_length(Channel channel, uint16_t pattern, uint8_t length);
static inline void draw_channel_with_playhead(Channel channel, uint16_t pattern, uint8_t length, uint8_t position);
static void draw_active_channel_display();
static inline uint8_t anim_dazzle(uint8_t frame, uint8_t x, uint8_t y);
static inline uint8_t anim_marching_ants(uint8_t frame, uint8_t x, uint8_t y);
/// Read a single step from a pattern
/// @param pattern The pattern to read from, stored as 16 bitflags.
/// @param length The length of the pattern. Must be <= 16.
/// @param position The step at which to read. Must be < `length`.
/// @return `true` if there is an active step at this position, `false` otherwise.
static bool pattern_read(uint16_t pattern, uint8_t length, uint8_t position);
int encoder_read(Encoder& enc);
static void eeprom_load(EuclideanState *s);
static inline int eeprom_addr_length(Channel channel);
static inline int eeprom_addr_density(Channel channel);
static inline int eeprom_addr_offset(Channel channel);
static void active_channel_set(Channel channel);
static uint8_t output_channel_led_x(OutputChannel channel);
#define framebuffer_pixel_on(x, y) (framebuffer_pixel_set(x, y, COLOR_ON))
#define framebuffer_pixel_on_fast(x, y) (framebuffer_pixel_set_fast(x, y, COLOR_ON))
#define framebuffer_pixel_off(x, y) (framebuffer_pixel_set(x, y, COLOR_OFF))
#define framebuffer_pixel_off_fast(x, y) (framebuffer_pixel_set_fast(x, y, COLOR_OFF))
/// Set a single pixel on the framebuffer to the 2-bit color, using a coordinate 
/// system that is not mirrored left-to-right. Overwrites existing color.
/// @param x Zero-indexed position, from left to right.
/// @param y Zero-indexed position, from top to bottom.
/// @param color 2-bit color. `COLOR_OFF` or `0` turns off pixel, `COLOR_ON` 
/// or `1` turns it on.
static inline void framebuffer_pixel_set(uint8_t x, uint8_t y, Color color);
/// Like `framebuffer_pixel_set()`, but does not overwrite the existing color - it 
/// is assumed to be `COLOR_OFF`. It also does not mark the row as needing a 
/// redraw. You can mark the row as needing a redraw by calling
/// `framebuffer_row_off()` before calling this function.
/// @param x Zero-indexed position, from left to right.
/// @param y Zero-indexed position, from top to bottom.
/// @param color 2-bit color. `COLOR_OFF` or `0` turns off pixel, `COLOR_ON` 
/// or `1` turns it on.
static inline void framebuffer_pixel_set_fast(uint8_t x, uint8_t y, Color color);
/// Clear a row of pixels on the framebuffer
/// @param y Zero-indexed position, from top to bottom.
#define framebuffer_row_off(y) (framebuffer_row_set(y, 0))
/// Set the color values directly for a row of pixels on the LED Matrix.
/// Colors are 2-bit.
/// @param y Zero-indexed position, from top to bottom.
static inline void framebuffer_row_set(uint8_t y, uint16_t pixels);
/// Copy one row of the framebuffer to the LED matrix. We only copy one row of 
/// the framebuffer to the LED matrix per cycle to avoid having to wait on the 
/// display driver chip.
static void framebuffer_copy_row_to_display();
void led_sleep();
void led_wake();
void led_anim_wake();
void led_anim_sleep();
void startUpOK();

/// Initialize the MAX72XX LED Matrix
void led_init(void) {
  // The LED matrix is in power-saving mode on startup.
  // Set power-saving mode to false to wake it up
  lc.shutdown(LED_ADDR, false);
  lc.setIntensity(LED_ADDR, LED_BRIGHTNESS);
  lc.clearDisplay(LED_ADDR);
}

/// Keep the data in the state in bounds. Bounds excursions can happen when 
/// loading from the EEPROM.
static void validate_euclidean_state(EuclideanState *s) {
  for (uint8_t c = 0; c < NUM_CHANNELS; c++) {
    if ((s->channels[c].length > BEAT_LENGTH_MAX) || (s->channels[c].length < BEAT_LENGTH_MIN)) {
      s->channels[c].length = BEAT_LENGTH_DEFAULT;
    }
    if (s->channels[c].density > BEAT_DENSITY_MAX) {
      s->channels[c].density = BEAT_DENSITY_DEFAULT;
    }
    if (s->channels[c].offset > BEAT_OFFSET_MAX) {
      s->channels[c].offset = BEAT_OFFSET_DEFAULT;
    }
    if (s->channels[c].position > BEAT_POSITION_MAX) {
      s->channels[c].position = BEAT_POSITION_DEFAULT;
    }
  }
}

/// Turn on pull-up resistors for encoders
void encoders_init(void) {
  digitalWrite(PIN_ENC_1A, HIGH);
  digitalWrite(PIN_ENC_1B, HIGH);
  digitalWrite(PIN_ENC_2A, HIGH);
  digitalWrite(PIN_ENC_2B, HIGH);
  digitalWrite(PIN_ENC_3A, HIGH);
  digitalWrite(PIN_ENC_3B, HIGH);
}

void serial_init(void) {
  #if LOGGING_ENABLED
  Serial.begin(9600);
  #endif
}

/// Set up IO pins
void io_pins_init(void) {
  pinMode(PIN_IN_TRIG, INPUT);

  pinMode(PIN_OUT_CHANNEL_1, OUTPUT);
  pinMode(PIN_OUT_CHANNEL_2, OUTPUT);
  pinMode(PIN_OUT_CHANNEL_3, OUTPUT);
  pinMode(PIN_OUT_OFFBEAT, OUTPUT);
}

/* MAIN */

void setup() {
  time = millis();
  timeout_reset(&led_sleep_timeout, time);

  led_init();
  eeprom_load(&euclidean_state);
  validate_euclidean_state(&euclidean_state);
  encoders_init();
  serial_init();
  io_pins_init();

  // Initialise beat holders
  for (int a = 0; a < NUM_CHANNELS; a++) {
    generated_rhythms[a] = euclidean_pattern_rotate(euclidean_state.channels[a].length, euclidean_state.channels[a].density, euclidean_state.channels[a].offset);
  }

  startUpOK();

  led_wake();

  // Select first channel on startup
  active_channel_set(CHANNEL_1);
  draw_active_channel_display();

  // Draw all channel displays initially
  for (uint8_t channel = 0; channel < NUM_CHANNELS; channel++) {
    uint8_t length = euclidean_state.channels[channel].length;
    uint16_t pattern = generated_rhythms[channel];
    uint8_t position = euclidean_state.channels[channel].position;
    draw_channel_with_playhead((Channel) channel, pattern, length, position);
  }
}

void loop() {
  time = millis();

  #if LOGGING_ENABLED && LOGGING_CYCLE_TIME
  Microseconds cycle_time_start = micros();
  #endif

  /* INPUT EVENTS */

  InputEvents events_in = INPUT_EVENTS_EMPTY;

  // READ TRIG AND RESET INPUTS
  int trig_in_value = digitalRead(PIN_IN_TRIG);
  int reset_button = analogRead(A1);

  // RESET INPUT & BUTTON
  if ((!reset_active) && (reset_button >= RESET_PIN_THRESHOLD)) {
    reset_active = true;

    events_in.reset = true;

    #if LOGGING_ENABLED && LOGGING_INPUT
    Serial.println("INPUT: Reset");
    #endif
  }
  if (reset_active && (reset_button < RESET_PIN_THRESHOLD)) {
    reset_active = false;
  }

  // TRIG INPUT 
  if (trig_in_value > trig_in_value_previous) { 
    events_in.trig = true;

    #if LOGGING_ENABLED && LOGGING_INPUT
    Serial.println("INPUT: Trigger");
    #endif
  }
  trig_in_value_previous = trig_in_value;

  // ENCODER MOVEMENT
  if (timeout_fired(&encoder_read_timeout, time)) {
    // Encoder 1: LENGTH (CH1)
    int val_enc_1 = encoder_read(Enc1);
    if (val_enc_1 != 0) {
      timeout_reset(&encoder_read_timeout, time);
      events_in.enc_move[ENCODER_1] = val_enc_1;

      #if LOGGING_ENABLED && LOGGING_INPUT
      Serial.print("ENC_1: Move ");
      Serial.println(val_enc_1);
      #endif
    }

    // Encoder 2: DENSITY (CH2)
    int val_enc_2 = encoder_read(Enc2);
    if (val_enc_2 != 0) {
      timeout_reset(&encoder_read_timeout, time);
      events_in.enc_move[ENCODER_2] = val_enc_2;

      #if LOGGING_ENABLED && LOGGING_INPUT
      Serial.print("ENC_2: Move ");
      Serial.println(val_enc_2);
      #endif
    }

    // Encoder 3: OFFSET (CH3)
    int val_enc_3 = encoder_read(Enc3);
    if (val_enc_3 != 0) {
      timeout_reset(&encoder_read_timeout, time);
      events_in.enc_move[ENCODER_3] = val_enc_3;

      #if LOGGING_ENABLED && LOGGING_INPUT
      Serial.print("ENC_3: Move ");
      Serial.println(val_enc_3);
      #endif
    }
  }

  // ENCODER PUSHES
  int channel_switch_val = analogRead(PIN_IN_CHANNEL_SWITCH);
  bool enc_pushed;
  EncoderIdx enc_idx;
  if (channel_switch_val < 100) {
    // Nothing pushed
    enc_pushed = false;
    enc_idx = ENCODER_NONE;
    channelPressedCounter = 0;
  } else if (channel_switch_val < 200) {
    // Density pushed
    enc_pushed = true;
    enc_idx = ENCODER_2;
    channelPressedCounter++;
  } else if (channel_switch_val < 400) {
    // Length pushed
    enc_pushed = true;
    enc_idx = ENCODER_1;
    channelPressedCounter++;
  } else {
    // Offset pushed
    enc_pushed = true;
    enc_idx = ENCODER_3;
    channelPressedCounter++;
  }

  if (enc_pushed && (channelPressedCounter <= 1)) {
    events_in.enc_push = enc_idx;
  }

  /* HANDLE INPUT */

  // Handle Encoder Pushes
  switch (events_in.enc_push) {
    case ENCODER_1:
      active_channel_set(CHANNEL_2);
      break;
    case ENCODER_2:
      active_channel_set(CHANNEL_3);
      break;
    case ENCODER_3:
      active_channel_set(CHANNEL_1);
      break;
    default:
      break;
  }

  EuclideanParam param_changed = EUCLIDEAN_PARAM_NONE;

  // Handle Length Knob Movement
  int nknob = events_in.enc_move[ENCODER_1];
  if (nknob != 0) {
    param_changed = EUCLIDEAN_PARAM_LENGTH;

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
      EEPROM.update(eeprom_addr_density(channel), density);
      #endif
    }
    if ((offset >= (length + nknob)) && (offset < 16)) {
      offset += nknob;
      euclidean_state.channels[channel].offset = offset;

      #if EEPROM_WRITE
      EEPROM.update(eeprom_addr_offset(channel), offset);
      #endif
    }

    length += nknob;
    euclidean_state.channels[channel].length = length;

    // Reset position if length has been reduced past it
    if (position >= length) {
      euclidean_state.channels[channel].position = 0;
    }
    
    #if EEPROM_WRITE
    EEPROM.update(eeprom_addr_length(channel), length);
    #endif
      
    #if LOGGING_ENABLED && EEPROM_WRITE
    Serial.print("eeprom write N= ");
    Serial.print((channel * 2) + 1);
    Serial.print(" ");
    Serial.println(length);
    #endif
  }

  // Handle Density Knob Movement
  int kknob = events_in.enc_move[ENCODER_2];
  if (kknob != 0) {
    param_changed = EUCLIDEAN_PARAM_DENSITY;

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
    EEPROM.update(eeprom_addr_density(channel), density);
    #endif

    #if LOGGING_ENABLED && EEPROM_WRITE
    Serial.print("eeprom write K= ");
    Serial.print((channel * 2) + 2);
    Serial.print(" ");
    Serial.println(density);
    #endif
  }

  // Handle Offset Knob Movement
  int oknob = events_in.enc_move[ENCODER_3];
  if (oknob != 0) {
    param_changed = EUCLIDEAN_PARAM_OFFSET;

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
    EEPROM.update(eeprom_addr_offset(channel), offset);
    #endif

    #if LOGGING_ENABLED && EEPROM_WRITE
    Serial.print("eeprom write O= ");
    Serial.print((channel) + 7);
    Serial.print(" ");
    Serial.println(offset);
    #endif
  }

  // Update Generated Rhythms Based On Parameter Changes
  if (param_changed != EUCLIDEAN_PARAM_NONE) {
    Channel channel = active_channel;
    EuclideanChannelState channel_state = euclidean_state.channels[channel];
    uint8_t length = channel_state.length;
    uint8_t density = channel_state.density;
    uint8_t offset = channel_state.offset;

    generated_rhythms[channel] = euclidean_pattern_rotate(length, density, offset);

    #if LOGGING_ENABLED
    if (param_changed == EUCLIDEAN_PARAM_LENGTH) {
      Serial.print("length: ");
      Serial.println(length);
    } else if (param_changed == EUCLIDEAN_PARAM_DENSITY) {
      Serial.print("density: ");
      Serial.println(density);
    } else {
      Serial.print("offset: ");
      Serial.println(offset);
    }
    #endif
  }

  /* UPDATE INTERNAL CLOCK */

  // Turn off internal clock when external clock received
  if (events_in.trig) { 
    internal_clock_enabled = false; 
  }

  if (events_in.reset) {
    timeout_reset(&internal_clock_timeout, time);
  }

  if (internal_clock_enabled && (timeout_loop(&internal_clock_timeout, time))) {
    events_in.internal_clock_tick = true;
  }

  /* UPDATE SEQUENCER */

  // Clock ticks merge the internal and external clocks
  bool clock_tick = events_in.trig || events_in.internal_clock_tick;

  // Tracks if any of the sequencers' states have been updated this cycle
  bool sequencers_updated = (clock_tick || events_in.reset);

  // Bitflags storing which output channels will fire this cycle, indexed by
  // `OutputChannel`.
  uint8_t out_channels_firing = 0;

  if (events_in.reset) {
    sequencer_handle_reset();
  }

  if (clock_tick) {
    sequencer_handle_clock();

    out_channels_firing = sequencer_read_current_step();
  }

  /* OUTPUT */

  for (uint8_t out_channel = 0; out_channel < OUTPUT_NUM_CHANNELS; out_channel++) {
    bool should_fire = out_channels_firing & (0x01 << out_channel);
    if (should_fire) {
      output_set_high((OutputChannel)out_channel);
    }
  }

  if (clock_tick || events_in.reset) {
    // Update output pulse length and timeout
    Milliseconds pulse_length = constrain(((time - output_pulse_timeout.start) / 5), 2, 5);
    output_pulse_timeout.duration = pulse_length;

    timeout_reset(&output_pulse_timeout, time);
  }

  // FINISH ANY PULSES THAT ARE ACTIVE
  if (output_any_active() && (timeout_fired(&output_pulse_timeout, time))) {
    output_clear_all();
  }

  /* DRAWING - UPDATE ANIMATIONS */
  if(timeout_loop(&anim_dazzle_timeout, time)) {
    anim_dazzle_frame = (anim_dazzle_frame + 1) % ANIM_DAZZLE_NUM_FRAMES;
  }

  if(timeout_loop(&anim_ants_timeout, time)) {
    anim_ants_frame = (anim_ants_frame + 1) % ANIM_ANTS_NUM_FRAMES;
  }

  /* DRAWING - OUTPUT INDICATORS */

  // If the sequencer has moved, note active output channels and blink 
  // output indicators
  if (clock_tick | events_in.reset) {
    for (uint8_t out_channel = 0; out_channel < OUTPUT_NUM_CHANNELS; out_channel++) {
      uint8_t x = output_channel_led_x((OutputChannel)out_channel);

      uint8_t mask = 0x01 << out_channel;
      bool active_step_prev = output_channels_active_step_bitflags & mask;
      bool active_step = out_channels_firing & mask;

      if (active_step != active_step_prev) {
        // Toggle output channel as having an active step in the bitflags w/ XOR
        output_channels_active_step_bitflags ^= mask;
      } else {
        framebuffer_pixel_off(x, LED_OUT_Y);
      }
    }

    timeout_once_reset(&output_indicator_blink_timeout, time);
  }

  // Draw Output Indicators
  if (timeout_once_fired(&output_indicator_blink_timeout, time)) {
    for (uint8_t out_channel = 0; out_channel < OUTPUT_NUM_CHANNELS; out_channel++) {
      uint8_t x = output_channel_led_x((OutputChannel)out_channel);
      
      uint8_t active_step = (output_channels_active_step_bitflags >> out_channel) & 0x01;

      framebuffer_pixel_set(x, LED_OUT_Y, (Color)active_step);
    }
  }

  /* DRAWING - INPUT INDICATORS */

  // Flash Trig indicator LED if we received a clock tick
  if (clock_tick) {
    framebuffer_pixel_on(LED_IN_TRIG_X, LED_OUT_Y);
    timeout_once_reset(&trig_indicator_timeout, time);
  }

  // Flash Reset indicator LED if we received a reset input event
  if (events_in.reset) {
    framebuffer_pixel_on(LED_IN_RESET_X, LED_OUT_Y);
    timeout_once_reset(&reset_indicator_timeout, time);
  }
  
  // Turn off indicator LEDs that have been on long enough
  if (timeout_once_fired(&trig_indicator_timeout, time)) {
    framebuffer_pixel_off(LED_IN_TRIG_X, LED_OUT_Y);
  }
  if (timeout_once_fired(&reset_indicator_timeout, time)) {
    framebuffer_pixel_off(LED_IN_RESET_X, LED_OUT_Y);
  }

  /* DRAWING - ACTIVE CHANNEL DISPLAY */

  if (events_in.enc_push != ENCODER_NONE) {
    draw_active_channel_display();  
  }

  /* DRAWING - CHANNELS */

  if (sequencers_updated) {
    // Update playhead blink duration based on the last interval between two
    // clock or reset signals received.
    Milliseconds previous_period = time - last_clock_or_reset;
    playhead_blink_timeout.inner.duration = calc_playhead_blink_time(previous_period);
    last_clock_or_reset = time;

    // Reset playhead blink
    timeout_once_reset(&playhead_blink_timeout, time);

    #if PLAYHEAD_IDLE
    // Reset playhead idle
    timeout_reset(&playhead_idle_timeout, time);
    #endif
  }

  // Update playhead idle - Make playhead blink periodically when it hasn't 
  // moved in a certain amount of time
  bool playhead_blink_updated = false;
  #if PLAYHEAD_IDLE
  if (timeout_fired(&playhead_idle_timeout, time)) {
    if (timeout_loop(&playhead_idle_loop_timeout, time)) {
      playhead_blink_timeout.inner.duration = PLAYHEAD_BLINK_TIME_DEFAULT;
      timeout_once_reset(&playhead_blink_timeout, time);
      playhead_blink_updated = true;
    }
  }
  #endif

  // Update playhead blink
  if (timeout_once_fired(&playhead_blink_timeout, time)) {
      playhead_blink_updated = true;
  }

  // Tracks if the screen needs to be redrawn. 
  bool needs_redraw = sequencers_updated || playhead_blink_updated;

  if (param_changed == EUCLIDEAN_PARAM_NONE) {
    // If no parameters have changed, check if the adjustment display still 
    // needs to be shown, and hide it if it doesn't
    if (adjustment_display_state.visible) {
      bool should_be_hidden = timeout_fired(&adjustment_display_timeout, time);
      if (should_be_hidden) {
        adjustment_display_state.visible = false;
        needs_redraw = true;
      }
    }
  } else {
    // If parameters have changed, reset the adjustment display timeout and state
    adjustment_display_state.channel = active_channel;
    adjustment_display_state.parameter = param_changed;
    adjustment_display_state.visible = true;
    timeout_reset(&adjustment_display_timeout, time);

    needs_redraw = true;
  }

  if (needs_redraw) {
    draw_channels();
  }

  /* UPDATE LED DISPLAY */

  framebuffer_copy_row_to_display();

  /* UPDATE LED SLEEP */

  if (input_events_contains_any_external(&events_in)) {
    timeout_reset(&led_sleep_timeout, time);
  }
  if (led_sleep_mode_active) {
    // LED is sleeping:
    // If it has been less than LED_SLEEP_TIME since an interaction event has
    // been received, wake the LED
    if (!timeout_fired(&led_sleep_timeout, time)) {
      led_wake();
      draw_channels();
      draw_active_channel_display();
    }
  } else {
    // LED is awake:
    // Sleep it if no inputs have been received or generated since LED_SLEEP_TIME ago
    if (timeout_fired(&led_sleep_timeout, time)) {
      led_sleep();
    }
  }


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

static bool input_events_contains_any_external(InputEvents *events) {
  bool result = (
    events->trig ||
    events->reset ||
    (events->enc_push != ENCODER_NONE) ||
    (events->enc_move[CHANNEL_1] != 0) ||
    (events->enc_move[CHANNEL_2] != 0) ||
    (events->enc_move[CHANNEL_3] != 0)
  );
  return result;
}

static Milliseconds calc_playhead_blink_time(Milliseconds clock_period) {
  // 256ms period = ~234bpm
  // 1280ms period = ~47bpm
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

static void sequencer_handle_reset() {
  // Go to the first step for each channel
  for (uint8_t channel = 0; channel < NUM_CHANNELS; channel++) {
    euclidean_state.channels[channel].position = 0;
  }

  // Stop the sequencer
  euclidean_state.sequencer_running = false;
}

static void sequencer_handle_clock() {
  // Advance sequencer if it is running
  if (euclidean_state.sequencer_running) {
    // Only advance if sequencer is running
    sequencer_advance();
  } else {
    // If sequencer is stopped, start it so that the next clock advances
    euclidean_state.sequencer_running = true;
  }
}

static void sequencer_advance() {
  for (uint8_t channel = 0; channel < NUM_CHANNELS; channel++) {
    EuclideanChannelState channel_state = euclidean_state.channels[channel];
    uint8_t length = channel_state.length;
    uint8_t position = channel_state.position;

    // Move sequencer playhead to next step
    position++;
    if (position >= length) {
      position = 0;
    }
    euclidean_state.channels[channel].position = position;

    #if LOGGING_ENABLED && LOGGING_POSITION
    if (channel == 0) {
      Serial.print("> CH_1 Position: ");
      Serial.println(position);
    }
    #endif
  }
}

static uint8_t sequencer_read_current_step() {
  uint8_t out_channels_firing = 0;

  for (uint8_t channel = 0; channel < NUM_CHANNELS; channel++) {
    EuclideanChannelState channel_state = euclidean_state.channels[channel];
    uint8_t length = channel_state.length;
    uint8_t position = channel_state.position;
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

static void draw_channels() {
  for (uint8_t channel = 0; channel < NUM_CHANNELS; channel++) {
    draw_channel((Channel)channel);
  }
}

static inline void draw_channel(Channel channel) {
  EuclideanChannelState channel_state = euclidean_state.channels[channel];
  uint8_t length = channel_state.length;
  uint8_t position = channel_state.position;
  uint16_t pattern = generated_rhythms[channel];

  // Clear rows
  uint8_t row = channel * 2;
  framebuffer_row_off(row);
  framebuffer_row_off(row + 1);

  bool showing_length_display = (adjustment_display_state.visible) && 
                                (channel == adjustment_display_state.channel) &&
                                (adjustment_display_state.parameter == EUCLIDEAN_PARAM_LENGTH);
  if (showing_length_display) { 
    draw_channel_length(channel, pattern, length);  
  }
  draw_channel_with_playhead(channel, pattern, length, position);
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

static inline void draw_channel_with_playhead(Channel channel, uint16_t pattern, uint8_t length, uint8_t position) {
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
    bool playhead_blink_active = playhead_blink_timeout.active;
    Color color = (active_step ^ (playhead_here && playhead_blink_active)) ? COLOR_ON : COLOR_OFF;

    framebuffer_pixel_set_fast(x, y, color);
  }
}

static void draw_active_channel_display() {
    uint16_t row_bits = 0;
    if (active_channel == CHANNEL_1) {
      row_bits = 0x0005; // Two left dots
    } else if (active_channel == CHANNEL_2) {
      row_bits = 0x0140; // Two middle dots
    } else if (active_channel == CHANNEL_3) {
      row_bits = 0x5000; // Two right dots
    } 
    framebuffer_row_set(LED_CH_SEL_Y, row_bits);
}

static inline uint8_t anim_dazzle(uint8_t frame, uint8_t x, uint8_t y) {
  return ((x + y + frame) % ANIM_DAZZLE_NUM_FRAMES);
}

static inline uint8_t anim_marching_ants(uint8_t frame, uint8_t x, uint8_t y) {
  uint8_t val = (x + y + (ANIM_ANTS_NUM_FRAMES - frame)) / 2;
  return (val % 2);
}

static bool pattern_read(uint16_t pattern, uint8_t length, uint8_t position) {
  uint8_t idx = length - position - 1;
  return (pattern >> idx) & 0x01;
}

/* Read an encoder
  returns +1, 0 or -1 dependent on direction
  Contains no internal debounce, so calls should be delayed
*/
int encoder_read(Encoder& enc) {
  int result = 0;
  int32_t value_read = enc.read();
  if (value_read == 0) {
    enc.write(0);
    result = 0;
  } else if (value_read < -2) {
    result = -1;
    enc.write(0);
  } else if (value_read > 2) {
    result = 1;
    enc.write(0);
  }
  return result;
}

static void active_channel_set(Channel channel) {
  // Update state
  active_channel = channel;
  
  #if LOGGING_ENABLED
  Serial.print("Active channel: ");
  Serial.println(active_channel);
  #endif
}

static uint8_t output_channel_led_x(OutputChannel channel) {
  uint8_t result;
  if (channel == OUTPUT_CHANNEL_1) {
    result = LED_OUT_CH1_X;
  } else if (channel == OUTPUT_CHANNEL_2) {
    result = LED_OUT_CH2_X;
  } else if (channel == OUTPUT_CHANNEL_3) {
    result = LED_OUT_CH3_X;
  } else {
    result = LED_OUT_OFFBEAT_X;
  }
  return result;
}

static inline void framebuffer_pixel_set(uint8_t x, uint8_t y, Color color) {
  // Clear existing color
  uint16_t mask = 0x0003; // Must be 16 bits because it gets inverted
  framebuffer[y] &= ~(mask << (x * 2));

  // Set new color
  framebuffer[y] |= (color << (x * 2));
}

static inline void framebuffer_pixel_set_fast(uint8_t x, uint8_t y, Color color) {
  // Set new color
  framebuffer[y] |= (color << (x * 2));
}

static inline void framebuffer_row_set(uint8_t y, uint16_t pixels) {
  framebuffer[y] = pixels;
}

static void framebuffer_copy_row_to_display() {
  uint8_t row = (framebuffer_out_row) % LED_ROWS;
  uint16_t fb_row_bits = framebuffer[row];

  uint8_t to_draw = 0;
  for (uint8_t col = 0; col < LED_COLUMNS; col++) {
    Color color = (Color)((fb_row_bits >> (col * 2)) & 0b00000011);

    if (color == COLOR_ANTS) {
      to_draw |= (anim_marching_ants(anim_ants_frame, col, row) << col);
    } else if (color == COLOR_DAZZLE) {
      to_draw |= (anim_dazzle(anim_dazzle_frame, col, row) << col);
    } else {
      to_draw |= (color << col);
    }
  }

  lc.setRow(LED_ADDR, row, to_draw);

  // Next cycle, copy the next row of the framebuffer to the LED matrix
  framebuffer_out_row = (framebuffer_out_row + 1) % LED_ROWS;
}

/// Load state from EEPROM into the given `EuclideanState`
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

static inline int eeprom_addr_length(Channel channel) {
  return (channel * 2) + 1;
}

static inline int eeprom_addr_density(Channel channel) {
  return (channel * 2) + 2;
}

static inline int eeprom_addr_offset(Channel channel) {
  return channel + 7;
}

void led_sleep() {
  led_sleep_mode_active = true;

  led_anim_sleep();
  lc.shutdown(LED_ADDR, true);
}

void led_wake() {
  led_sleep_mode_active = false;

  lc.shutdown(LED_ADDR, false);
  led_anim_wake();
}

void led_anim_wake() { 
  for (uint8_t step = 0; step < 4; step++) {
    uint8_t a = 3 - step;
    lc.setRow(LED_ADDR, a, 255);
    lc.setRow(LED_ADDR, 7 - a, 255);
    delay(100);
    lc.setRow(LED_ADDR, a, 0);
    lc.setRow(LED_ADDR, 7 - a, 0);
  }
}

void led_anim_sleep() {
  for (uint8_t a = 0; a < 4; a++) {
    lc.setRow(LED_ADDR, a, 255);
    lc.setRow(LED_ADDR, 7 - a, 255);
    delay(200);
    lc.setRow(LED_ADDR, a, 0);
    lc.setRow(LED_ADDR, 7 - a, 0);
  }
}

void startUpOK() {
  digitalWrite(PIN_OUT_CHANNEL_3, HIGH);
  delay(50);
  digitalWrite(PIN_OUT_CHANNEL_3, LOW);
  delay(200);
  digitalWrite(PIN_OUT_CHANNEL_3, HIGH);
  delay(50);
  digitalWrite(PIN_OUT_CHANNEL_3, LOW);
  delay(200);
  digitalWrite(PIN_OUT_CHANNEL_3, HIGH);
  delay(50);
  digitalWrite(PIN_OUT_CHANNEL_3, LOW);
  delay(200);
  digitalWrite(PIN_OUT_CHANNEL_3, HIGH);
  delay(50);
  digitalWrite(PIN_OUT_CHANNEL_3, LOW);
  delay(200);
}
