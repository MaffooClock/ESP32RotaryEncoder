/**
 * ESP32RotaryEncoder: LeftOrRight.ino
 * 
 * This is a simple example of how to track whether the knob was
 * turned left or right instead of tracking a numeric value
 * 
 * Created 1 November 2023
 * By Matthew Clark
 */

#include <ESP32RotaryEncoder.h>


// Change these to the actual pin numbers that
// you've connected your rotary encoder to
const uint8_t DI_ENCODER_A   = 27;
const uint8_t DI_ENCODER_B   = 14;
const int8_t  DI_ENCODER_SW  = 12;
const int8_t  DO_ENCODER_VCC = 13;


RotaryEncoder rotaryEncoder( DI_ENCODER_A, DI_ENCODER_B, DI_ENCODER_SW, DO_ENCODER_VCC );


// Used by the `loop()` to know when to
// fire  an event when the knob is turned
volatile bool turnedRightFlag = false;
volatile bool turnedLeftFlag = false;

void turnedRight()
{
	Serial.println( "Right ->" );

	// Set this back to false so we can watch for the next move
	turnedRightFlag = false;
}

void turnedLeft()
{
	Serial.println( "<- Left" );

	// Set this back to false so we can watch for the next move
	turnedLeftFlag = false;
}

void knobCallback( long value )
{
	// See the note in the `loop()` function for
	// an explanation as to why we're setting
	// boolean values here instead of running
	// functions directly.

	// Don't do anything if either flag is set;
	// it means we haven't taken action yet
	if( turnedRightFlag || turnedLeftFlag )
		return;

	// Set a flag that we can look for in `loop()`
	// so that we know we have something to do
	switch( value )
	{
		case 1:
	  		turnedRightFlag = true;
		break;

		case -1:
	  		turnedLeftFlag = true;
		break;
	}

	// Override the tracked value back to 0 so that
	// we can continue tracking right/left events
	rotaryEncoder.setEncoderValue( 0 );
}

void buttonCallback( unsigned long duration )
{
	Serial.printf( "boop! button was down for %lu ms\n", duration );
}

void setup()
{
	Serial.begin( 115200 );

	// This tells the library that the encoder has its own pull-up resistors
	rotaryEncoder.setEncoderType( EncoderType::HAS_PULLUP );

	// The encoder will only return -1, 0, or 1, and will not wrap around.
	rotaryEncoder.setBoundaries( -1, 1, false );

	// The function specified here will be called every time the knob is turned
	// and the current value will be passed to it
	rotaryEncoder.onTurned( &knobCallback );

	// The function specified here will be called every time the button is pushed and
	// the duration (in milliseconds) that the button was down will be passed to it
	rotaryEncoder.onPressed( &buttonCallback );

	// This is where the inputs are configured and the interrupts get attached
	rotaryEncoder.begin();
}

void loop()
{
	// Check to see if a flag is set (is true), and if so, run a function.
	//
	// Why do it like this instead of within the `knobCallback()` function?
	//
	// Because the `knobCallback()` function is executed by a internal
	// timer ISR (interrupt service routine), which needs to be fast and
	// lean, so setting a boolean is the fastest way, and then let the main
	// `loop()` do the heavy-lifting.
	//
	// If you were to let the `knobCallback()` function do all the work,
	// there's a chance you'd have issues with WiFi or Bluetooth connections,
	// or even cause the MCU to crash.

	if( turnedRightFlag )
		turnedRight();

	else if( turnedLeftFlag )
		turnedLeft();
}
