# platformio prototype

This folder contains startup code for an ESP01s generated and built
by platformio. It was initially created by running `pio init -b esp01_1m`.

To compile the code and upload it first time (when it does not have the
arduino over-the-air code: `pio run -t upload -c platformio-serial.ini`.

To compile the code and upload it to the device when it already has the
arduino over-the-air code: `pio run -t upload`.

# Installation

* In VS Code, search for package `platformio` and install it.
* Create softlinks to command line tools somewhere in your path
  (ie `~/.local/bin`). The tools can be found in `~/.platformio/penv/bin`:
  - `pio`
  - `piodebuggdb`
  - `platformio`

# Building

* In code: Ctrl-Alt-b
* Command line: `pio run`
