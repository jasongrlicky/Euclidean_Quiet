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
    - Output indicator LEDs now stay lit for the entire duration of the step.
    - LED sleep timeout now takes into account encoder manipulations
  - UI Polish:
    - Reset is now visible immediately
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
#define LED_OUT_TRIG_X 0
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

/// Stores each generated Euclidean rhythm as 16 bits. Indexed by channel number.
uint16_t generated_rhythms[NUM_CHANNELS];
Channel active_channel; // Channel that is currently active
static Timeout internal_clock_timeout = { .duration = INTERNAL_CLOCK_PERIOD };
static Timeout output_pulse_timeout = { .duration = 50 }; // Pulse length, set based on the time since last trigger

static Timeout trig_indicator_timeout = { .duration = INPUT_INDICATOR_FLASH_TIME }; // Set based on the time since last trigger
bool trig_indicator_active = false;
/// Stores which output channels have active steps this step of their sequencer,
/// as bitflags indexted by `OutputChannel`.
uint8_t output_channels_active_step_bitflags = 0;
static Timeout output_indicator_blink_timeout = { .duration = 16 };

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

#define REDRAW_MASK_NONE  0b00000000
#define REDRAW_MASK_ALL   0b00000111

/* INTERNAL */

/// Returns true if `events` contains any externally-generated events
static bool input_events_contains_any_external(InputEvents *events);
static void sequencer_handle_reset();
static void sequencer_handle_clock();
static void sequencer_advance();
/// What is the output that should be sent for each sequencer this cycle
/// @return Bitflags, indexed using `OutputChannel`. 1 = begin an output pulse this cycle for this channel, 0 = do nothing for this channel 
static uint8_t sequencer_read_current_step();
static void draw_channels();
static inline void draw_channel(Channel channel);
static inline void draw_channel_length(Channel channel, uint8_t length);
static inline void draw_channel_with_playhead(Channel channel, uint16_t pattern, uint8_t length, uint8_t position);
static inline void draw_channel_playhead(uint8_t y, uint8_t position);
static void draw_channel_pattern(Channel channel, uint16_t pattern, uint8_t length);
static void draw_active_channel_display();
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
#define led_pixel_on(x, y) (led_pixel_set(x, y, true))
#define led_pixel_off(x, y) (led_pixel_set(x, y, false))
/// Set a single pixel on the LED Matrix to be on or off, using a coordinate 
/// system that is not mirrored left-to-right.
/// @param x Zero-indexed position, from left to right.
/// @param y Zero-indexed position, from top to bottom.
/// @param val If `true`, lights pixel. If `false`, turns off pixel.
static inline void led_pixel_set(uint8_t x, uint8_t y, bool val);
/// Clear a row of pixels on the LED Matrix.
/// @param y Zero-indexed position, from top to bottom.
static inline void led_row_off(uint8_t y);
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
    draw_channel_pattern((Channel) channel, pattern, length);
  }
}

void loop() {
  time = millis();

  /* INPUT EVENTS */

  InputEvents events_in = INPUT_EVENTS_EMPTY;

  // READ TRIG AND RESET INPUTS
  int trig_in_value = digitalRead(PIN_IN_TRIG); // Pulse input
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
      Serial.print(length);
    } else if (param_changed == EUCLIDEAN_PARAM_DENSITY) {
      Serial.print(" density: ");
      Serial.print(density);
    } else {
      Serial.print(" offset: ");
      Serial.print(offset);
    }
    #endif
  }

  /* UPDATE INTERNAL CLOCK */

  // Turn off internal clock when external clock received
  if (events_in.trig) { 
    internal_clock_enabled = false; 
  }

  if (internal_clock_enabled && (timeout_fired(&internal_clock_timeout, time))) {
    events_in.internal_clock_tick = true;
  }

  if (events_in.internal_clock_tick || events_in.reset) {
    timeout_reset(&internal_clock_timeout, time);
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

  /* DRAWING - INDICATORS */

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
        led_pixel_off(x, LED_OUT_Y);
      }
    }

    timeout_reset(&output_indicator_blink_timeout, time);
  }

  // Draw Output Indicators
  if (timeout_fired(&output_indicator_blink_timeout, time)) {
    for (uint8_t out_channel = 0; out_channel < OUTPUT_NUM_CHANNELS; out_channel++) {
      uint8_t x = output_channel_led_x((OutputChannel)out_channel);
      
      bool active_step = output_channels_active_step_bitflags & (0x01 << out_channel);

      led_pixel_set(x, LED_OUT_Y, active_step);
    }
  }

  // Flash Trig indicator LED if we received a clock tick
  if (clock_tick) {
    led_pixel_on(LED_OUT_TRIG_X, LED_OUT_Y);
    trig_indicator_active = true;
    timeout_reset(&trig_indicator_timeout, time);
  }
  
  // Turn off indicator LEDs that have been on long enough
  if (trig_indicator_active && (timeout_fired(&trig_indicator_timeout, time))) {
    led_pixel_off(LED_OUT_TRIG_X, LED_OUT_Y);
    trig_indicator_active = false;
  }

  /* DRAWING - ACTIVE CHANNEL DISPLAY */

  if (events_in.enc_push != ENCODER_NONE) {
    draw_active_channel_display();  
  }

  /* DRAWING - CHANNELS */

  // Tracks if the screen needs to be redrawn. 
  bool needs_redraw = sequencers_updated;

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

  if (adjustment_display_state.visible && (channel == adjustment_display_state.channel)) { 
    if (adjustment_display_state.parameter == EUCLIDEAN_PARAM_LENGTH) {
      draw_channel_length(channel, length);  
    } else {
      draw_channel_pattern(channel, pattern, length);
    }
  } else {
    draw_channel_with_playhead(channel, pattern, length, position);
  }
}

static inline void draw_channel_length(Channel channel, uint8_t length) {
    uint8_t row = channel * 2;
    led_row_off(row);
    led_row_off(row + 1);

    for (uint8_t step = 0; step < length; step++) {
      uint8_t x = step;
      uint8_t y = row;
      if (step > 7) {
        x -= 8;
        y += 1;
      }

      led_pixel_set(x, y, true);
    }
}

static inline void draw_channel_with_playhead(Channel channel, uint16_t pattern, uint8_t length, uint8_t position) {
  uint8_t y = channel * 2;
  led_row_off(y);

  if (position < 8) {
    for (uint8_t step = 0; step < 8; step++) {
      if (pattern_read(pattern, length, step) && (step < length)) {
        led_pixel_on(step, y);
      }
    }
  } else {
    for (uint8_t step = 8; step < 16; step++) {
      if (pattern_read(pattern, length, step) && (step < length)) {
        led_pixel_on(step - 8, y);
      }
    }
  }

  draw_channel_playhead(y + 1, position);
}

static inline void draw_channel_playhead(uint8_t y, uint8_t position) {
  led_row_off(y);
  uint8_t x = (position < 8) ? position : position - 8;
  led_pixel_on(x, y);
}

static void draw_channel_pattern(Channel channel, uint16_t pattern, uint8_t length) {
    uint8_t row = channel * 2;
    led_row_off(row);
    led_row_off(row + 1);

    for (uint8_t step = 0; step < length; step++) {
      uint8_t x = step;
      uint8_t y = row;
      if (step > 7) {
        x -= 8;
        y += 1;
      }

      if (pattern_read(pattern, length, step)) {
        led_pixel_set(x, y, true);
      }
    }
}

static void draw_active_channel_display() {
    uint8_t row_bits = B00000000;
    if (active_channel == CHANNEL_1) {
      row_bits = B00000011;
    } else if (active_channel == CHANNEL_2) {
      row_bits = B00011000;
    } else if (active_channel == CHANNEL_3) {
      row_bits = B11000000;
    } 
    lc.setRow(LED_ADDR, LED_CH_SEL_Y, row_bits);
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

static inline void led_pixel_set(uint8_t x, uint8_t y, bool val) {
  lc.setLed(LED_ADDR, y, 7 - x, val);
}

static inline void led_row_off(uint8_t y) {
  lc.setRow(LED_ADDR, y, 0);
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
