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

# Hardware

The ESP01s is put on a small board that gets its power from the HAN port. Since it is 5V, a 3.3V regulator is added on the board.

The serial line of the HAN port is inverted, so the code inverts the UART in `han_setup()`.

```

+---------------------------------------+
|                                       |
|            U0Rxd, GPIO3  7 ---O   O --|--8 Vcc
|                                       |
|          SPI_CS2, GPIO0  5 ---O   O --|--6 Ext_RST
|                                       |
|       LED, U1Txd, GPIO2  3 ---O   O --|--4 Chip_EN, CH_PD
|                                       |
|                     GND  1 ---O   O --|--2 GPIO1, U0Txd, SPI_CS1
|                                       |
+---------------------------------------+
```

Connected pins:
* 1 - GND
* 8 - Vcc
* 7 - UART Rx for data, with pullup to 3.3V
* 5 - GPIO output
