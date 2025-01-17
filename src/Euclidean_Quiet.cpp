#include <EEPROM.h>
#include <Encoder.h>
#include <LedControl.h>

extern "C" {
#include "config.h"
#include <euclidean.h>
#include "hardware.h"
#include "output.h"
#include "types.h"
}

/* 
  Alternate "Quiet" Firmware for Sebsongs Modular Euclidean Eurorack module.

  Changes from official v1.2 firmware:
  - Behavior changes:
    - Channel 1 is now selected when the module starts up
    - The internal clock no longer starts when the module starts up.
    - Patterns generated are now accurate to the original Euclidean Rhythms paper.
  - UI Polish:
    - The "Trig" LED indicator now illuminates every clock pulse instead of alternating ones.
    - Made channel selection easier to see (two dots instead of 4 overlapping).
    - Shortened the time that LEDs stay lit when setting pattern length for a channel.
  - Bugs Fixed:
    - The internal clock started up again when the reset button was pressed.
    - Reset did not function for any channel if channel 1's playhead was at position 0.
    - Validating faulty saved data did not happen until after that data was used.
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

/// State of the Euclidean rhythm generator and sequencer for a single channel
typedef struct EuclideanChannel {
  /// Number of steps in the Euclidean rhythm, 1-16
  uint8_t length;
  /// Number of active steps in the Euclidean rhythm, 1-16
  uint8_t density;
  /// Number of steps to rotate the Euclidean rhythm to the right, 1-16
  uint8_t offset;
  /// Step index representing the playhead position for this channel's sequencer, 0-15
  uint8_t position; 
} EuclideanChannel;

/// References one of the three channels
typedef enum Channel {
  CHANNEL_1,
  CHANNEL_2,
  CHANNEL_3,
} Channel;

/// State of the entire Euclidean module
typedef struct EuclideanState {
  /// State for each of this module's channels, indexed by `Channel` enum.
  EuclideanChannel channels[NUM_CHANNELS];
} EuclideanState;

static EuclideanState euclidean_state;

Milliseconds time;
Milliseconds last_clock;
#if LOGGING_ENABLED
Milliseconds last_logged;
#endif

// Stores each generated Euclidean rhythm as 16 bits. Indexed by channel number.
uint16_t generated_rhythms[NUM_CHANNELS];

Channel active_channel; // Channel that is currently active
Milliseconds output_pulse_length = 50; // Pulse length, set based on the time since last trigger
bool lights_active = false;
bool led_sleep_mode_enabled = true;

int trig_in_value_previous = 0; // For recognizing trigger in rising edges
bool reset_active = false;
unsigned long channelPressedCounter = 0;
Milliseconds last_read;
Milliseconds last_changed;

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
  bool trig_rise;
  /// "Reset" input or button detected a rising edge
  bool reset_rise;
  /// The internal clock generated a tick
  bool internal_clock_tick;
} InputEvents;

/// An instance of `InputEvents` which represents no events happening
static const InputEvents INPUT_EVENTS_EMPTY = {
  .enc_move = { 0, 0, 0 },
  .enc_push = ENCODER_NONE,
  .trig_rise = false,
  .reset_rise = false,
  .internal_clock_tick = false,
};

/// A parameter was changed for the Euclidean rhythm generator
enum EuclideanParamChange {
  EUCLIDEAN_PARAM_CHANGE_NONE,
  EUCLIDEAN_PARAM_CHANGE_LENGTH,
  EUCLIDEAN_PARAM_CHANGE_DENSITY,
  EUCLIDEAN_PARAM_CHANGE_OFFSET,
};

/* INTERNAL */

void handle_clock();
static void draw_channel(Channel channel, uint16_t pattern, uint8_t length, uint8_t position);
/// Read a single step from a pattern
/// @param pattern The pattern to read from, stored as 16 bitflags.
/// @param length The length of the pattern. Must be <= 16.
/// @param position The step at which to read. Must be < `length`.
/// @return `true` if there is an active step at this position, `false` otherwise.
static bool pattern_read(uint16_t pattern, uint8_t length, uint8_t position);
int encoder_read(Encoder& enc);
static void active_channel_set(Channel channel);
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
    s->channels[c].length = EEPROM.read((c << 1) + 1);
    s->channels[c].density = EEPROM.read((c << 1) + 2);
    s->channels[c].offset = EEPROM.read(c + 7);
    s->channels[c].position = 0;
  }
  #endif
}

/// Keep the data in the state in bounds. Bounds excursions can happen when 
/// loading from the EEPROM.
static void validate_euclidean_state(EuclideanState *s) {
  for (uint8_t c = 0; c < NUM_CHANNELS; c++) {
    if (s->channels[c].length > BEAT_LENGTH_MAX) {
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

  handle_clock();

  // Select first channel on startup
  active_channel_set(CHANNEL_1);
}

void loop() {
  time = millis();

  /* INPUT EVENTS */

  InputEvents events_in = INPUT_EVENTS_EMPTY;

  // READ TRIG AND RESET INPUTS
  int trig_in_value = digitalRead(PIN_IN_TRIG); // Pulse input
  int reset_button = analogRead(A1);

  // RESET INPUT & BUTTON
  if ((!reset_active) && (reset_button > RESET_PIN_THRESHOLD)) {
    reset_active = true;

    events_in.reset_rise = true;

    #if LOGGING_ENABLED
    Serial.println("INPUT: Reset Rise");
    #endif
  }
  if (reset_active && (reset_button < RESET_PIN_THRESHOLD)) {
    reset_active = false;

    #if LOGGING_ENABLED
    Serial.println("INPUT: Reset Fall");      
    #endif
  }

  // TRIG INPUT 
  if (trig_in_value > trig_in_value_previous) { 
    events_in.trig_rise = true;

    #if LOGGING_ENABLED
    Serial.println("INPUT: Trigger Rise");
    #endif
  }
  trig_in_value_previous = trig_in_value;

  // ENCODER MOVEMENT
  if (time - last_read > READ_DELAY) {
    // Encoder 1: LENGTH (CH1)
    int val_enc_1 = encoder_read(Enc1);
    if (val_enc_1 != 0) {
      last_read = time;
      events_in.enc_move[ENCODER_1] = val_enc_1;

      #if LOGGING_ENABLED
      Serial.print("ENC_1: Move ");
      Serial.println(nknob);
      #endif
    }

    // Encoder 2: DENSITY (CH2)
    int val_enc_2 = encoder_read(Enc2);
    if (val_enc_2 != 0) {
      last_read = time;
      events_in.enc_move[ENCODER_2] = val_enc_2;

      #if LOGGING_ENABLED
      Serial.print("ENC_2: Move ");
      Serial.println(kknob);
      #endif
    }

    // Encoder 3: OFFSET (CH3)
    int val_enc_3 = encoder_read(Enc3);
    if (val_enc_3 != 0) {
      last_read = time;
      events_in.enc_move[ENCODER_3] = val_enc_3;

      #if LOGGING_ENABLED
      Serial.print("ENC_3: Move ");
      Serial.println(oknob);
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

  /* INTERNAL EVENTS */

  // Internal Clock
  if (internal_clock_enabled && (time - last_clock > INTERNAL_CLOCK_PERIOD)) {
    events_in.internal_clock_tick = true;
  }

  /* UPDATE STATE */

  // Internal Clock
  if (events_in.internal_clock_tick) {
    handle_clock();
  }

  // Sleep the LED matrix if no clock has been received or generated since LED_SLEEP_TIME
  if ((!led_sleep_mode_enabled) && (time - last_clock > LED_SLEEP_TIME)) {
    led_sleep();
  }

  // HANDLE RESET
  if (events_in.reset_rise) {
    for (uint8_t a = 0; a < NUM_CHANNELS; a++) {
      euclidean_state.channels[a].position = 0;
    }

    if(led_sleep_mode_enabled) {
      handle_clock();
    }
  }

  // HANDLE TRIGGER INPUT
  if (events_in.trig_rise) { 
    internal_clock_enabled = false; // turn off internal clock if external clock received
    handle_clock();
  }
  
  // TURN OFF ANY LIGHTS THAT ARE ON
  if (lights_active && (time - last_clock > output_pulse_length)) {
    led_row_off(7);
    lights_active = false;
  }

  // FINISH ANY PULSES THAT ARE ACTIVE
  if (output_any_active() && (time - last_clock > output_pulse_length)) {
    output_clear_all();
  }

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

  if (euclidean_state.channels[active_channel].length > 16) {
    euclidean_state.channels[active_channel].length = 16;
  }

  // Local copies of active channel parameters
  int active_length = euclidean_state.channels[active_channel].length;
  int active_density = euclidean_state.channels[active_channel].density;
  int active_offset = euclidean_state.channels[active_channel].offset;

  EuclideanParamChange param_changed = EUCLIDEAN_PARAM_CHANGE_NONE;

  // Handle Density Knob Movement
  int kknob = events_in.enc_move[ENCODER_2];
  if (kknob) {
    param_changed = EUCLIDEAN_PARAM_CHANGE_DENSITY;

    if (euclidean_state.channels[active_channel].density + kknob > euclidean_state.channels[active_channel].length) {
      kknob = 0;
    } // check within limits
    if (euclidean_state.channels[active_channel].density + kknob < BEAT_DENSITY_MIN) {
      kknob = 0;
    }

    // CHECK AGAIN FOR LOGIC
    if (euclidean_state.channels[active_channel].density > euclidean_state.channels[active_channel].length - 1) {
      euclidean_state.channels[active_channel].density = euclidean_state.channels[active_channel].length - 1;
    }

    euclidean_state.channels[active_channel].density = euclidean_state.channels[active_channel].density + kknob; // update with encoder reading
    #if EEPROM_WRITE
    EEPROM.update((active_channel * 2) + 2, euclidean_state.channels[active_channel].density); // write settings to 2/4/6 eproms
    #endif

    #if LOGGING_ENABLED
    Serial.print("eeprom write K= ");
    Serial.print((active_channel * 2) + 2);
    Serial.print(" ");
    Serial.println(euclidean_state.channels[active_channel].density);
    #endif
  }

  // Handle Length Knob Movement
  int nknob = events_in.enc_move[ENCODER_1];
  if (nknob != 0) {
    param_changed = EUCLIDEAN_PARAM_CHANGE_LENGTH;

    // Sense check n encoder reading to prevent crashes
    if (active_length >= BEAT_LENGTH_MAX) {
      active_length = BEAT_LENGTH_MAX;
    } // Check for eeprom values over maximum.
    if (active_length + nknob > BEAT_LENGTH_MAX) {
      nknob = 0;
    } // check below BEAT_LENGTH_MAX
    if (active_length + nknob < BEAT_LENGTH_MIN) {
      nknob = 0;
    } // check above BEAT_LENGTH_MIN

    if (active_density >= active_length + nknob && active_density > 1) {// check if new n is lower than k + reduce K if it is
      euclidean_state.channels[active_channel].density = euclidean_state.channels[active_channel].density + nknob;
    }

    if (active_offset >= active_length + nknob && active_offset < 16) {// check if new n is lower than o + reduce o if it is
      euclidean_state.channels[active_channel].offset = euclidean_state.channels[active_channel].offset + nknob;
      #if EEPROM_WRITE
      EEPROM.update((active_channel) + 7, euclidean_state.channels[active_channel].offset); // write settings to 2/4/6 eproms
      #endif
    }

    euclidean_state.channels[active_channel].length = active_length + nknob; // update with encoder reading
    active_density = euclidean_state.channels[active_channel].density;
    active_length = euclidean_state.channels[active_channel].length;  // update for ease of coding
    active_offset = euclidean_state.channels[active_channel].offset;
    
    #if EEPROM_WRITE
    EEPROM.update((active_channel * 2) + 1, euclidean_state.channels[active_channel].length); // write settings to 2/4/6 eproms
    #endif
      
    #if LOGGING_ENABLED
    Serial.print("eeprom write N= ");
    Serial.print((active_channel * 2) + 1);
    Serial.print(" ");
    Serial.println(euclidean_state.channels[active_channel].length);
    #endif
  }

  // Handle Offset Knob Movement
  int oknob = events_in.enc_move[ENCODER_3];
  if (oknob != 0) {
    param_changed = EUCLIDEAN_PARAM_CHANGE_OFFSET;

    // Sense check o encoder reading to prevent crashes
    if (active_offset + oknob > active_length - 1) {
      oknob = 0;
    } // check below BEAT_OFFSET_MAX
    if (active_offset + oknob < BEAT_OFFSET_MIN) {
      oknob = 0;
    } // check above BEAT_LENGTH_MIN

    euclidean_state.channels[active_channel].offset = active_offset + oknob;
    active_offset = euclidean_state.channels[active_channel].offset;  // update active_offset for ease of coding

    #if EEPROM_WRITE
    EEPROM.update((active_channel) + 7, euclidean_state.channels[active_channel].offset); // write settings to 2/4/6 eproms
    #endif

    #if LOGGING_ENABLED
    Serial.print("eeprom write O= ");
    Serial.print((active_channel) + 7);
    Serial.print(" ");
    Serial.println(euclidean_state.channels[active_channel].offset);
    #endif
  }

  // UPDATE BEAT HOLDER WHEN KNOBS ARE MOVED
  if (param_changed != EUCLIDEAN_PARAM_CHANGE_NONE) {
    generated_rhythms[active_channel] = euclidean_pattern_rotate(active_length, active_density, active_offset);
    led_row_off(active_channel * 2);
    led_row_off(active_channel * 2 + 1);

    if (param_changed == EUCLIDEAN_PARAM_CHANGE_LENGTH) { 
      // Length changed - Display total length of beat
      for (uint8_t step = 0; step < active_length; step++) {
        uint8_t x = step;
        uint8_t y = active_channel * 2;
        if (step > 7) {
          x -= 8;
          y += 1;
        }

        led_pixel_set(x, y, true);
      }
    } else {  
      // Density or Offset changed -  Display beats in the active channel
      for (uint8_t step = 0; step < active_length; step++) {
        if (pattern_read(generated_rhythms[active_channel], active_length, step)) {
          uint8_t x = step;
          uint8_t y = active_channel * 2;
          if (step > 7) {
            x -= 8;
            y += 1;
          }

          led_pixel_set(x, y, true);
        }
      }
    }

    last_changed = time;
  }

  // Log parameters at a certain interval
  #if LOGGING_ENABLED
  if (time - last_logged > LOGGING_INTERVAL) {
    last_logged = time;
    Serial.print("length =");
    Serial.print(active_length);
    Serial.print(" density =");
    Serial.print(active_density);
    Serial.print(" offset =");
    Serial.print(active_offset);
  }
  #endif
}

// Triggered when clock pulses are received via the "Trig" input or generated 
// internally
void handle_clock() {
  // wake up routine & animation
  if (led_sleep_mode_enabled) {
    led_wake();
  }

  // Flash LED in bottom-left corner. It will get turned off with the rest of
  // the LEDs on the bottom row in the loop() function.
  lc.setLed(LED_ADDR, 7, 7, true);

  // Update each channel's sequencer
  for (uint8_t channel = 0; channel < NUM_CHANNELS; channel++) {
    uint8_t length = euclidean_state.channels[channel].length;
    uint8_t position = euclidean_state.channels[channel].position;
    uint16_t pattern = generated_rhythms[channel];

    draw_channel((Channel)channel, pattern, length, position);
  
    // Turn on LEDs on the bottom row for channels where the step is active
    bool step_is_active = pattern_read(pattern, length, position);
    if (step_is_active) {
      output_set_high((OutputChannel)channel);

      if (channel == CHANNEL_1) {
        led_pixel_on(2, 7);
      }
      if (channel == CHANNEL_2) {
        led_pixel_on(5, 7);
      }
      if (channel == CHANNEL_3) {
        led_pixel_on(7, 7);
      }

      lights_active = true;
    }

    // Create output pulses for Offbeat Channel - inverse of Channel 1
    if ((channel == CHANNEL_1) && (!step_is_active)) {
      output_set_high(OUTPUT_CHANNEL_OFFBEAT);
      
      led_pixel_on(3, 7); // bottom row flash
      lights_active = true;
    }

    // Move sequencer playhead to next step
    uint8_t new_position = euclidean_state.channels[channel].position;
    new_position++;
    if (new_position >= euclidean_state.channels[channel].length) {
      new_position = 0;
    }
    euclidean_state.channels[channel].position = new_position;
  }

  output_pulse_length = constrain(((time - last_clock) / 5), 2, 5);

  last_clock = time;
}

static void draw_channel(Channel channel, uint16_t pattern, uint8_t length, uint8_t position) {
  // don't clear or draw cursor if channel is being changed
  if ((channel != active_channel) || (time - last_changed > ADJUSTMENT_DISPLAY_TIME)) {
    led_row_off(channel * 2);

    if (position < 8) {
      for (uint8_t step = 0; step < 8; step++) {
        if (pattern_read(pattern, length, step) && (step < length)) {
          led_pixel_on(step, channel * 2);
        }
      }
    } else {
      for (uint8_t step = 8; step < 16; step++) {
        if (pattern_read(pattern, length, step) && (step < length)) {
          led_pixel_on(step - 8, channel * 2);
        }
      }
    }

    led_row_off(channel * 2 + 1);
    // Draw sequencer playhead
    if (position < 8) {
      led_pixel_on(position, (channel * 2) + 1);
    } else {
      led_pixel_on(position - 8, (channel * 2) + 1);
    }
  }
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
    
    // Update LED Matrix
    uint8_t row_bits = B00000000;
    if (channel == CHANNEL_1) {
      row_bits = B00000011;
    } else if (channel == CHANNEL_2) {
      row_bits = B00011000;
    } else if (channel == CHANNEL_3) {
      row_bits = B11000000;
    } 
    lc.setRow(LED_ADDR, 6, row_bits);
}

static inline void led_pixel_set(uint8_t x, uint8_t y, bool val) {
  lc.setLed(LED_ADDR, y, 7 - x, val);
}

static inline void led_row_off(uint8_t y) {
  lc.setRow(LED_ADDR, y, 0);
}

void led_sleep() {
  led_sleep_mode_enabled = true;

  led_anim_sleep();
  lc.shutdown(LED_ADDR, true);
}

void led_wake() {
  led_sleep_mode_enabled = false;

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
