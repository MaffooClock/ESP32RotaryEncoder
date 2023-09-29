# ESP32RotaryEncoder

[![arduino-library-badge](https://www.ardu-badge.com/badge/ESP32RotaryEncoder.svg?)](https://www.ardu-badge.com/ESP32RotaryEncoder) [![License](https://img.shields.io/badge/license-MIT%20License-blue.svg)](http://doge.mit-license.org)

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

I will soon be publishing this to the [PlatformIO Registry](https://registry.platformio.org/) and [Arduino Library Manager](https://docs.arduino.cc/software/ide-v1/tutorials/installing-libraries#using-the-library-manager).

In the meantime, if you want to use this library, you'll need to install it manually.  Once I've got them published, I'll update this section.

### PlatformIO

Edit your [platformio.ini](https://docs.platformio.org/en/latest/projectconf/index.html) file, and add this repo to your [`lib_deps`](https://docs.platformio.org/en/latest/projectconf/sections/env/options/library/lib_deps.html) stanza.

If you're not familiar with editing your platformio.ini file manually, it should look something like this:
```ini
[env:arduino_nano_esp32]
platform = espressif32
board = arduino_nano_esp32
framework = arduino
lib_deps = 
	https://github.com/MaffooClock/ESP32RotaryEncoder.git
```

Alternatively, you can [download the ZIP](https://github.com/MaffooClock/ESP32RotaryEncoder/archive/refs/heads/main.zip) and un-zip it inside the `lib` directory inside your project.

### Arduino IDE

See their documentation on [Importing a .zip Library](https://docs.arduino.cc/software/ide-v1/tutorials/installing-libraries#importing-a-zip-library).


## Usage

Adding a rotary encoder instance is easy:

```c++
#include <Arduino.h>
#include <ESP32RotaryEncoder.h>

#define DO_ENCODER_VCC D2
#define DI_ENCODER_SW  D3
#define DI_ENCODER_B   D4 // DT
#define DI_ENCODER_A   D5 // CLK


RotaryEncoder rotaryEncoder( DI_ENCODER_A, DI_ENCODER_B, DI_ENCODER_SW, DO_ENCODER_VCC );


void knobCallback( int value )
{
	Serial.printf( PSTR("Value: %i\n"), value );
}

void buttonCallback()
{
	Serial.println( PSTR("boop!") );
}

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

	// The function specified here will be called every time the button is pushed
	rotaryEncoder.onPressed( &buttonCallback );

	// This is where the inputs are configured and the interrupts get attached
	rotaryEncoder.begin();
}

void loop()
{
	// Your stuff here
}
```

There are other options and methods you can call, but this is just the most basic implementation.

> **Warning**
> Keep the `onTurned()` and `onPressed()` callbacks lightweight, and definitely _do not_ use any calls to `delay()` here.  If you need to do some heavy lifting or use delays, it's better to set a flag here, then check for that flag in your `loop()` and run the appropriate functions from there.


## Compatibility

So far, this has only been tested on an [Arduino Nano ESP32](https://docs.arduino.cc/hardware/nano-esp32).  This _should_ work on any ESP32 in Arduino IDE and PlatformIO as long as your framework packages are current.

This library more than likely won't work at all on non-ESP32 devices -- it uses features from the ESP32 IDF, such as [esp_timer.h](https://github.com/espressif/esp-idf/blob/master/components/esp_timer/include/esp_timer.h), along with [FunctionalInterrupt.h](https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/FunctionalInterrupt.h) from the Arduino API.  So, to try and use this on a non-ESP32 might require some serious overhauling.


## Examples

_Coming soon!_

