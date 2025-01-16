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
  - Internal:
    - Migrated firmware project to PlatformIO.
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
#define BEAT_LENGTH_MAX 16
#define BEAT_LENGTH_MIN 1
// K: Density
#define BEAT_DENSITY_MIN 0
// O: Offset
#define BEAT_OFFSET_MIN 0

/* GLOBALS */

bool internal_clock_enabled = false;

// Initialize objects for reading encoders
// (from the Encoder.h library)
Encoder Enc1(PIN_ENC_2B, PIN_ENC_2A); // Length  / N
Encoder Enc2(PIN_ENC_1B, PIN_ENC_1A); // Density / K
Encoder Enc3(PIN_ENC_3B, PIN_ENC_3A); // Offset  / O

// Initialize objects for controlling LED matrix
// (from LedControl.h library)
// 1 is maximum number of devices that can be controlled
LedControl lc = LedControl(PIN_OUT_LED_DATA, PIN_OUT_LED_CLOCK, PIN_OUT_LED_SELECT, 1);

uint8_t channelbeats[NUM_CHANNELS][5] = {
  {
    16, 4, 0, 0, 0
  }
  , {
    16, 4, 0, 0, 0
  }
  , {
    16, 4, 0, 0, 0
  }
};

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

/// State of the entire Euclidean module
typedef struct EuclideanState {
  /// State for each of this module's channels
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

uint8_t active_channel; // Which channel is active? zero indexed
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
int encoder_read(Encoder& enc);
void active_channel_set(uint8_t channel);
void led_sleep();
void led_wake();
void led_anim_wake();
void led_anim_sleep();
void startUpOK();
static void channelbeats_from_state_channel(uint8_t channelbeats_channel[5], EuclideanChannel channel);

/// Initialize the MAX72XX LED Matrix
void led_init(void) {
  // The LED matrix is in power-saving mode on startup.
  // Set power-saving mode to false to wake it up
  lc.shutdown(LED_ADDR, false);
  lc.setIntensity(LED_ADDR, LED_BRIGHTNESS);
  lc.clearDisplay(LED_ADDR);
}

/// If there is faulty data in the eeprom, clear it and reset to default values
void eeprom_init(void) {
  #if EEPROM_READ && EEPROM_WRITE
  if ((EEPROM.read(1) > 16) || (EEPROM.read(2) > 16) || (EEPROM.read(3) > 16) ||
      (EEPROM.read(4) > 16) || (EEPROM.read(5) > 16) || (EEPROM.read(6) > 16) ||
      (EEPROM.read(7) > 15) || (EEPROM.read(8) > 15) || (EEPROM.read(9) > 15)) {
    // write a 0 to all bytes of the EEPROM
    for (int i = 0; i < 1024; i++){
      EEPROM.write(i, 0);
    }
      
    EEPROM.write(1, 16);
    EEPROM.write(2, 4);
    EEPROM.write(3, 16);
    EEPROM.write(4, 4);
    EEPROM.write(5, 16);
    EEPROM.write(6, 4);
    EEPROM.write(7, 0);
    EEPROM.write(8, 0);
    EEPROM.write(9, 0);
    EEPROM.write(10, 0);
    EEPROM.write(11, 0);
    EEPROM.write(12, 0);
  }
  #endif
}

/// Load state from EEPROM into the given `EuclideanState`
static void eeprom_load(EuclideanState *s) {
  /*
  EEPROM Schema:
  Channel 1: length = 1 density = 2 offset = 7
  Channel 2: length = 3 density = 4 offset = 8
  Channel 3: length = 5 density = 6 offset = 9
  */
  for (uint8_t c = 0; c < NUM_CHANNELS; c++) {
    s->channels[c].length = EEPROM.read((c << 1) + 1);
    s->channels[c].density = EEPROM.read((c << 1) + 2);
    s->channels[c].offset = EEPROM.read(c + 7);
    s->channels[c].position = 0;
  }

  // TEMPORARY: Convert EuclideanState to channelbeats
  for (uint8_t c = 0; c < NUM_CHANNELS; c++) {
    channelbeats_from_state_channel(channelbeats[c], s->channels[c]);
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
  eeprom_init();
  eeprom_load(&euclidean_state);
  encoders_init();
  serial_init();
  io_pins_init();

  // Initialise beat holders
  for (int a = 0; a < NUM_CHANNELS; a++) {
    generated_rhythms[a] = euclidean_pattern_rotate(channelbeats[a][0], channelbeats[a][1], channelbeats[a][3]);
  }

  startUpOK();

  handle_clock();

  // Select first channel on startup
  active_channel_set(0);
}

void loop() {
  /*
    What's in the loop:
    Update time variable
    Check to see if it is time go go to sleep
    Changes routine - update generated_rhythms when channelbeats changes - triggered by changes == true
    Trigger routines - on trigget update displays and pulse
    Read encoders
    Read switches
  */

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
      channelbeats[a][2] = 0;
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
    lc.setRow(LED_ADDR, 7, 0); // Clear row
    lights_active = false;
  }

  // FINISH ANY PULSES THAT ARE ACTIVE
  if (output_any_active() && (time - last_clock > output_pulse_length)) {
    output_clear_all();
  }

  // Handle Encoder Pushes
  switch (events_in.enc_push) {
    case ENCODER_1:
      active_channel_set(1);
      break;
    case ENCODER_2:
      active_channel_set(2);
      break;
    case ENCODER_3:
      active_channel_set(0);
      break;
    default:
      break;
  }

  if (channelbeats[active_channel][0] > 16) {
    channelbeats[active_channel][0] = 16;
  }

  // Local copies of active channel parameters
  int active_length = channelbeats[active_channel][0];
  int active_density = channelbeats[active_channel][1];
  int active_offset = channelbeats[active_channel][3];

  EuclideanParamChange param_changed = EUCLIDEAN_PARAM_CHANGE_NONE;

  // Handle Density Knob Movement
  int kknob = events_in.enc_move[ENCODER_2];
  if (kknob) {
    param_changed = EUCLIDEAN_PARAM_CHANGE_DENSITY;

    if (channelbeats[active_channel][1] + kknob > channelbeats[active_channel][0]) {
      kknob = 0;
    } // check within limits
    if (channelbeats[active_channel][1] + kknob < BEAT_DENSITY_MIN) {
      kknob = 0;
    }

    // CHECK AGAIN FOR LOGIC
    if (channelbeats[active_channel][1] > channelbeats[active_channel][0] - 1) {
      channelbeats[active_channel][1] = channelbeats[active_channel][0] - 1;
    }

    channelbeats[active_channel][1] = channelbeats[active_channel][1] + kknob; // update with encoder reading
    #if EEPROM_WRITE
    EEPROM.update((active_channel * 2) + 2, channelbeats[active_channel][1]); // write settings to 2/4/6 eproms
    #endif

    #if LOGGING_ENABLED
    Serial.print("eeprom write K= ");
    Serial.print((active_channel * 2) + 2);
    Serial.print(" ");
    Serial.println(channelbeats[active_channel][1]);
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
      channelbeats[active_channel][1] = channelbeats[active_channel][1] + nknob;
    }

    if (active_offset >= active_length + nknob && active_offset < 16) {// check if new n is lower than o + reduce o if it is
      channelbeats[active_channel][3] = channelbeats[active_channel][3] + nknob;
      #if EEPROM_WRITE
      EEPROM.update((active_channel) + 7, channelbeats[active_channel][3]); // write settings to 2/4/6 eproms
      #endif
    }

    channelbeats[active_channel][0] = active_length + nknob; // update with encoder reading
    active_density = channelbeats[active_channel][1];
    active_length = channelbeats[active_channel][0];  // update for ease of coding
    active_offset = channelbeats[active_channel][3];
    
    #if EEPROM_WRITE
    EEPROM.update((active_channel * 2) + 1, channelbeats[active_channel][0]); // write settings to 2/4/6 eproms
    #endif
      
    #if LOGGING_ENABLED
    Serial.print("eeprom write N= ");
    Serial.print((active_channel * 2) + 1);
    Serial.print(" ");
    Serial.println(channelbeats[active_channel][0]);
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

    channelbeats[active_channel][3] = active_offset + oknob;
    active_offset = channelbeats[active_channel][3];  // update active_offset for ease of coding

    #if EEPROM_WRITE
    EEPROM.update((active_channel) + 7, channelbeats[active_channel][3]); // write settings to 2/4/6 eproms
    #endif

    #if LOGGING_ENABLED
    Serial.print("eeprom write O= ");
    Serial.print((active_channel) + 7);
    Serial.print(" ");
    Serial.println(channelbeats[active_channel][3]);
    #endif
  }

  // UPDATE BEAT HOLDER WHEN KNOBS ARE MOVED
  if (param_changed != EUCLIDEAN_PARAM_CHANGE_NONE) {
    generated_rhythms[active_channel] = euclidean_pattern_rotate(active_length, active_density, active_offset);
    lc.setRow(LED_ADDR, active_channel * 2 + 1, 0);//clear active row
    lc.setRow(LED_ADDR, active_channel * 2, 0);//clear line above active row

    if (param_changed == EUCLIDEAN_PARAM_CHANGE_DENSITY) {  
      // Display beats in the active channel
      for (uint8_t a = 0; a < 8; a++) {
        if (bitRead(generated_rhythms[active_channel], active_length - 1 - a) == 1 && a < active_length) {
          lc.setLed(LED_ADDR, active_channel * 2, 7 - a, true);
        }
        if (bitRead(generated_rhythms[active_channel], active_length - 1 - a - 8) == 1 && a + 8 < active_length) {
          lc.setLed(LED_ADDR, active_channel * 2 + 1, 7 - a, true);
        }
      }
    } else if (param_changed == EUCLIDEAN_PARAM_CHANGE_LENGTH) { 
      // Display total length of beat
      for (uint8_t a = 0; a < 8; a++) {
        if (a < active_length) {
          lc.setLed(LED_ADDR, active_channel * 2, 7 - a, true);
        }
        if (a + 8 < active_length) {
          lc.setLed(LED_ADDR, active_channel * 2 + 1, 7 - a, true);
        }
      }
    } else if (param_changed == EUCLIDEAN_PARAM_CHANGE_OFFSET) {  
      // Display beats in the active channel
      for (uint8_t a = 0; a < 8; a++) {
        if (bitRead(generated_rhythms[active_channel], active_length - 1 - a) == 1 && a < active_length) {
          lc.setLed(LED_ADDR, active_channel * 2, 7 - a, true);
        }
        if (bitRead(generated_rhythms[active_channel], active_length - 1 - a - 8) == 1 && a + 8 < active_length) {
          lc.setLed(LED_ADDR, active_channel * 2 + 1, 7 - a, true);
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

  // Cycle through channels
  for (uint8_t a = 0; a < NUM_CHANNELS; a++) {
    int read_head = channelbeats[a][0] - channelbeats[a][2] - 1;

    // don't clear or draw cursor if channel is being changed
    if (a != active_channel || time - last_changed > ADJUSTMENT_DISPLAY_TIME) {
      lc.setRow(LED_ADDR, a * 2, 0);//clear line above active row

      if (channelbeats[a][2] < 8) {
        for (uint8_t c = 0; c < 8; c++) {
          if (bitRead(generated_rhythms[a], channelbeats[a][0] - 1 - c) == 1 && c < channelbeats[a][0]) {
            lc.setLed(LED_ADDR, a * 2, 7 - c, true);
          }
        }
      } else {
        for (uint8_t c = 8; c < 16; c++) {
          if (bitRead(generated_rhythms[a], channelbeats[a][0] - 1 - c) == 1 && c < channelbeats[a][0]) {
            lc.setLed(LED_ADDR, a * 2, 15 - c, true);
          }
        }
      }

      lc.setRow(LED_ADDR, a * 2 + 1, 0);//clear active row
      // draw cursor
      if (channelbeats[a][2] < 8) {
        lc.setLed(LED_ADDR, a * 2 + 1, 7 - channelbeats[a][2], true); // write cursor less than 8
      } else {
        if (channelbeats[a][2] < 16) {
          lc.setLed(LED_ADDR, a * 2 + 1, 15 - channelbeats[a][2], true); // write cursor more than 8
        }
      }
    }
    
    // turn on pulses on channels where a beat is present
    if (bitRead(generated_rhythms[a], read_head) == 1) {
      output_set_high((OutputChannel)a);

      if (a == 0) {
        lc.setLed(LED_ADDR, 7, 5, true);
      }
      if (a == 1) {
        lc.setLed(LED_ADDR, 7, 2, true);
      }
      if (a == 2) {
        lc.setLed(LED_ADDR, 7, 0, true);
      }

      lights_active = true;
    }

    // send off pulses to spare output for the first channel
    if (bitRead(generated_rhythms[a], read_head) == 0 && a == 0) { // only relates to first channel
      output_set_high(OUTPUT_CHANNEL_OFFBEAT);
      
      lc.setLed(LED_ADDR, 7, 4, true); // bottom row flash
      lights_active = true;
    }

    // move counter to next position, ready for next pulse
    channelbeats[a][2]++;
    if (channelbeats[a][2] >= channelbeats[a][0]) {
      channelbeats[a][2] = 0;
    }
  }

  output_pulse_length = constrain(((time - last_clock) / 5), 2, 5);

  last_clock = time;
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

void active_channel_set(uint8_t channel) {
    // Update state
    active_channel = channel;
    
    #if LOGGING_ENABLED
    Serial.print("Active channel: ");
    Serial.println(active_channel);
    #endif
    
    // Update LED Matrix
    uint8_t row_bits = B00000000;
    if (channel == 0) {
      row_bits = B00000011;
    } else if (channel == 1) {
      row_bits = B00011000;
    } else if (channel == 2) {
      row_bits = B11000000;
    } 
    lc.setRow(LED_ADDR, 6, row_bits);
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

static void channelbeats_from_state_channel(uint8_t channelbeats_channel[5], EuclideanChannel channel) {
  channelbeats_channel[0] = channel.length;
  channelbeats_channel[1] = channel.density;
  channelbeats_channel[2] = channel.position;
  channelbeats_channel[3] = channel.offset;
  channelbeats_channel[4] = 0;
}
