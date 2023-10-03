/**
 * ESP32RotaryEncoder: BasicRotaryEncoder.ino
 * 
 * This is a basic example of how to instantiate a single Rotary Encoder.
 * 
 * Turning the knob will increment/decrement a value between 1 and 10 and
 * print it to the serial console.
 * 
 * Pressing the button will output "boop!" to the serial console.
 * 
 * Created 3 October 2023
 * By Matthew Clark
 */

#include <ESP32RotaryEncoder.h>

#define DO_ENCODER_VCC D2
#define DI_ENCODER_SW  D3
#define DI_ENCODER_B   D4 // DT
#define DI_ENCODER_A   D5 // CLK


RotaryEncoder rotaryEncoder( DI_ENCODER_A, DI_ENCODER_B, DI_ENCODER_SW, DO_ENCODER_VCC );


void knobCallback( int value )
{
	Serial.printf( "Value: %i\n", value );
}

void buttonCallback()
{
	Serial.println( "boop!" );
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