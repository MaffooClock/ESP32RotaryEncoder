/**
 * ESP32RotaryEncoder: ButtonPressDuration.ino
 * 
 * This example shows how to handle long button-presses differently
 * from long button-presses
 * 
 * Turning the knob will increment/decrement a value between 1 and 10 and
 * print it to the serial console.
 * 
 * Pressing the button will output "boop!" to the serial console.
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

// A button-press is considered "long" if
// it's held for more than two seconds
const uint8_t LONG_PRESS = 2000;


RotaryEncoder rotaryEncoder( DI_ENCODER_A, DI_ENCODER_B, DI_ENCODER_SW, DO_ENCODER_VCC );


void buttonShortPress()
{
	Serial.println( "boop!" );
}

void buttonLongPress()
{
	Serial.println( "BOOOOOOOOOOP!" );
}

void knobCallback( long value )
{
	Serial.printf( "Value: %ld\n", value );
}

void buttonCallback( unsigned long duration )
{
	if( duration > LONG_PRESS )
	{
		buttonLongPress();
	}
	else
	{
		buttonShortPress();
	}
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

	// The function specified here will be called every time the button is pushed and
	// the duration (in milliseconds) that the button was down will be passed to it
	rotaryEncoder.onPressed( &buttonCallback );

	// This is where the inputs are configured and the interrupts get attached
	rotaryEncoder.begin();
}

void loop()
{
	// Your stuff here
}
