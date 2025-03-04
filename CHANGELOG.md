# Changelog

All notable changes to this project since the original firmware will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- Channel 1 is now selected when the module starts up.
- Before a clock trigger has been received, each channel's pattern is displayed.
- LED now dims itself before sleeping.
- There is now an indicator LED for Reset input, next to the one labeled "Trig".

### Changed

- A complete rewrite of the original firmware.
- Patterns generated are now accurate to the original Euclidean Rhythms paper.
- LED sleep timeout now takes into account encoder movement and presses.
- All steps of a generated pattern are now visible at all times - no more sequencer pages.
- The steps of a pattern are visible while adjusting its length.
- Steps of a pattern are now visible while setting pattern length.
- The effects of resetting the sequencers is now immediately visible.
- Output indicator LEDs now stay lit for the entire duration of the step.
- The "Trig" LED indicator now illuminates every clock pulse instead of alternating ones.
- Made channel selection easier to see (two dots instead of 4 overlapping).
- The adjustment display now hides itself after a certain amount of time, instead of waiting for the next clock signal.
- Migrated project to PlatformIO from Arduino IDE.
- Source code is now formatted by clang-format.
- Added automated tests for Euclidean rhythm generation algorithm.

### Removed

- The internal clock no longer starts when the module starts up.
- Remove LED sleep animations.
- Remove LED blink sequence on back of module when starting up.

### Fixed

- The internal clock started up again when the reset button was pressed.
- Sometimes ignored "Reset" input that happened simultaneously with "Trig" input.
- Turning the density up if it was already at the maximum would cause it to toggle between the two highest values.
- When the LED matrix wakes from sleep mode, the channel selection would not be displayed
- Reset did not function for any channel if channel 1's playhead was at position 0.
- Validating faulty saved data did not happen until after that data was used.
- When reducing the length parameter for a channel, its adjusted density would not be saved.
- When a pattern length was reduced to below the sequencer's current position, the position was not reset.
