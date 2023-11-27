# ESP32RotaryEncoder

[![Arduino Lint](https://github.com/MaffooClock/ESP32RotaryEncoder/actions/workflows/check-arduino.yml/badge.svg)](https://github.com/MaffooClock/ESP32RotaryEncoder/actions/workflows/check-arduino.yml) [![Compile Examples](https://github.com/MaffooClock/ESP32RotaryEncoder/actions/workflows/compile-examples.yml/badge.svg)](https://github.com/MaffooClock/ESP32RotaryEncoder/actions/workflows/compile-examples.yml) [![Arduino Library](https://www.ardu-badge.com/badge/ESP32RotaryEncoder.svg?)](https://www.ardu-badge.com/ESP32RotaryEncoder) [![PlatformIO Registry](https://badges.registry.platformio.org/packages/maffooclock/library/ESP32RotaryEncoder.svg)](https://registry.platformio.org/libraries/maffooclock/ESP32RotaryEncoder) [![License](https://img.shields.io/badge/license-MIT%20License-blue.svg)](http://doge.mit-license.org)

A simple Arduino library for implementing a rotary encoder on an ESP32 using interrupts and callbacks.


## Description

This library makes it easy to add one or more rotary encoders to your ESP32 project.  It uses interrupts to instantly detect when the knob is turned or the pushbutton is pressed and fire a custom callback to handle those events.

It works with assembled modules that include their own pull-up resistors as well as raw units without any other external components (thanks to the ESP32 having software controlled pull-up resistors built in).  You can also specify a GPIO pin to supply the Vcc reference instead of tying the encoder to a 3v3 source.

You can specify the boundaries of the encoder (minimum and maximum values), and whether turning past those limits should circle back around to the other side.


## Inspiration

There are already many rotary encoder libraries for Arduino, but I had trouble finding one that met my requirements.  I did find a couple that were beautifully crafted, but wouldn't work on the ESP32.  Others I tried were either bulky or clumsy, and I found myself feeling like it would be simpler to just setup the interrupts and handle the callbacks directly in my own code instead of through a library.

Of the many resources I used to educate myself on the best ways to handle the input from a rotary encoder was, the most notable one was [a blog post](https://garrysblog.com/2021/03/20/reliably-debouncing-rotary-encoders-with-arduino-and-esp32/) by [@garrysblog](https://github.com/garrysblog).  In his article, he cited another article, [Rotary Encoder: Immediately Tame your Noisy Encoder!](https://www.best-microcontroller-projects.com/rotary-encoder.html), which basically asserted that when turning the knob right or left, the pulses from the A and B pins can only happen in a specific order between detents, and any other pulses outside of that prescribed order could be ignored as noise.

Thus, by running the pulses received on the A and B inputs through a lookup table, it doesn't just de-bounce the inputs -- it actually guarantees that every click of the rotary encoder increments or decrements as the user would expect, regardless of how fast or slow or "iffy" the movement is.

Garry wrote some functions that incorporated the use of a lookup table, which is what I used myself initially -- and it worked _beautifully_.  In fact, it worked so well that I decided to turn it into a library to make it even simpler to use.  And that worked so well that I decided I should package it up and share it with others.


## Installation

#### PlatformIO

There are a few ways, choose whichever you prefer (pick **one**, don't do all three!):

- Search the [Library Registry](https://registry.platformio.org/search?t=library) for `MaffooClock/ESP32RotaryEncoder` and install it automatically.

- Edit your [platformio.ini](https://docs.platformio.org/en/latest/projectconf/index.html) file and add `MaffooClock/ESP32RotaryEncoder@^1.1.0` to your [`lib_deps`](https://docs.platformio.org/en/latest/projectconf/sections/env/options/library/lib_deps.html) stanza.

- Use the command line interface:
   ```shell
   cd MyProject
   pio pkg install --library "MaffooClock/ESP32RotaryEncoder@^1.1.0"
   ```

#### Arduino IDE

There are two ways (pick **one**, don't do both!):

- Search the Library Manager for `ESP32RotaryEncoder`

-  Manual install: download the [latest release](https://github.com/MaffooClock/ESP32RotaryEncoder/releases/latest), then see the documentation on [Importing a .zip Library](https://docs.arduino.cc/software/ide-v1/tutorials/installing-libraries#importing-a-zip-library).

### After Installation

Just add `include <ESP32RotaryEncoder.h>` to the top of your source file.


## Usage

Adding a rotary encoder instance is easy:

1. Include the library:

    ```c++
    #include <ESP32RotaryEncoder.h>
    ```


2. Define which pins to use, if you prefer to do it this way -- you could also just set the pins in the constructor (step 3):

    ```c++
    // Change these to the actual pin numbers that you've connected your rotary encoder to
    const int8_t  DO_ENCODER_VCC = D2; // Only needed if you're using a GPIO pin to supply the 3.3v reference
    const int8_t  DI_ENCODER_SW  = D3; // Pushbutton, if your rotary encoder has it
    const uint8_t DI_ENCODER_A   = D5; // Might be labeled CLK
    const uint8_t DI_ENCODER_B   = D4; // Might be labeled DT
    ```

3.  Instantiate a `RotaryEncoder` object:

    a)  This uses a GPIO pin to provide the 3.3v reference:
    ```c++
    RotaryEncoder rotaryEncoder( DI_ENCODER_A, DI_ENCODER_B, DI_ENCODER_SW, DO_ENCODER_VCC );
    ```

    b)  ...or you can free up the GPIO pin and tie Vcc to 3V3, then just omit that argument:
    ```c++
    RotaryEncoder rotaryEncoder( DI_ENCODER_A, DI_ENCODER_B, DI_ENCODER_SW );
    ```

    c)  ...or maybe your rotary encoder doesn't have a pushbutton?
    ```c++
    RotaryEncoder rotaryEncoder( DI_ENCODER_A, DI_ENCODER_B );
    ```

    d)  ...or you want to use a different library with the pushbutton, but still use a GPIO to provide the 3.3v reference:
    ```c++
    RotaryEncoder rotaryEncoder( DI_ENCODER_A, DI_ENCODER_B, -1, DO_ENCODER_VCC );
    ```

4. Add callbacks:

    ```c++
    void knobCallback( long value )
    {
        // This gets executed every time the knob is turned

        Serial.printf( "Value: %i\n", value );
    }

    void buttonCallback( unsigned long duration )
    {
        // This gets executed every time the pushbutton is pressed

        Serial.printf( "boop! button was down for %u ms\n", duration );
    }
    ```

5. Configure and initialize the `RotaryEncoder` object:

    ```c++
    void setup()
    {
        Serial.begin( 115200 );

        // This tells the library that the encoder has its own pull-up resistors
        rotaryEncoder.setEncoderType( EncoderType::HAS_PULLUP );

        // Range of values to be returned by the encoder: minimum is 1, maximum is 10
        // The third argument specifies whether turning past the minimum/maximum will
        // wrap around to the other side:
        //  - true  = turn past 10, wrap to 1; turn past 1, wrap to 10
        //  - false = turn past 10, stay on 10; turn past 1, stay on 1
        rotaryEncoder.setBoundaries( 1, 10, true );

        // The function specified here will be called every time the knob is turned
        // and the current value will be passed to it
        rotaryEncoder.onTurned( &knobCallback );

        // The function specified here will be called every time the button is pushed and
        // the duration (in milliseconds) that the button was down will be passed to it
        rotaryEncoder.onPressed( &buttonCallback );

        // This is where the inputs are configured and the interrupts get attached
        rotaryEncoder.begin();
    }
    ```

6. Done!  The library doesn't require you to do anything in `loop()`:

    ```c++
    void loop()
    {
        // Your stuff here
    }
    ```

There are other options and methods you can call, but this is just the most basic implementation.

> [!IMPORTANT]
> Keep the `onTurned()` and `onPressed()` callbacks lightweight, and definitely _do not_ use any calls to `delay()` here.  If you need to do some heavy lifting or use delays, it's better to set a flag here, then check for that flag in your `loop()` and run the appropriate functions from there.


## Debugging

This library makes use of the ESP32-IDF native logging to output some helpful debugging messages to the serial console.  To see it, you may have to add a build flag to set the logging level.  For PlatformIO, add `-DCORE_DEBUG_LEVEL=4` to the [`build_flags`](https://docs.platformio.org/en/stable/projectconf/sections/env/options/build/build_flags.html) option in [platformio.ini](https://docs.platformio.org/en/stable/projectconf/index.html).

After debugging, you can either remove the build flag (if you had to add it), or just reduce the level from debug (4) to info (3), warning (2), or error (1).  You can also use verbose (5) to get a few more messages beyond debug, but the overall output from sources other than this library might be noisy.

See [esp32-hal-log.h](https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/esp32-hal-log.h) for more details.


## Compatibility

So far, this has only been tested on an [Arduino Nano ESP32](https://docs.arduino.cc/hardware/nano-esp32).  This _should_ work on any ESP32 in Arduino IDE and PlatformIO as long as your framework packages are current.

This library more than likely won't work at all on non-ESP32 devices -- it uses features from the ESP32 IDF, such as [esp_timer.h](https://github.com/espressif/esp-idf/blob/master/components/esp_timer/include/esp_timer.h), along with [FunctionalInterrupt.h](https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/FunctionalInterrupt.h) from the Arduino API.  So, to try and use this on a non-ESP32 might require some serious overhauling.


## Examples

Check the [examples](/examples) folder.

