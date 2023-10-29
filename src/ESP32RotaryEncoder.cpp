#include "ESP32RotaryEncoder.h"

RotaryEncoder::RotaryEncoder( uint8_t encoderPinA, uint8_t encoderPinB, int8_t encoderPinButton, int8_t encoderPinVcc, uint8_t encoderSteps )
{
	this->encoderPinA      = encoderPinA;
	this->encoderPinB      = encoderPinB;
	this->encoderPinButton = encoderPinButton;
	this->encoderPinVcc    = encoderPinVcc;
	this->encoderTripPoint = encoderSteps - 1;
}

RotaryEncoder::~RotaryEncoder()
{
	detachInterrupts();

	esp_timer_stop( loopTimer );
	esp_timer_delete( loopTimer );
}

void RotaryEncoder::setEncoderType( EncoderType type )
{
	switch( type )
	{
		case FLOATING:
			encoderPinMode = INPUT_PULLUP;
			buttonPinMode  = INPUT_PULLUP;
		break;

		case HAS_PULLUP:
			encoderPinMode = INPUT;
			buttonPinMode  = INPUT;
		break;

		case SW_FLOAT:
			encoderPinMode = INPUT;
			buttonPinMode  = INPUT_PULLUP;
		break;
	}
}

void RotaryEncoder::setBoundaries( long minEncoderValue, long maxEncoderValue, bool circleValues )
{
	this->minEncoderValue = minEncoderValue;
	this->maxEncoderValue = maxEncoderValue;

	this->circleValues = circleValues;
}

void RotaryEncoder::onTurned( EncoderCallback f )
{
	callbackEncoderChanged = f;
}

void RotaryEncoder::onPressed( ButtonCallback f )
{
	callbackButtonPressed = f;
}

void RotaryEncoder::beginLoopTimer()
{
	/**
	 * We're using esp_timer.h from ESP32 SDK rather than esp32-hal-timer.h from Arduino API
	 * because the `timerAttachInterrupt()` won't accept std::bind like `attachInterrupt()` in
	 * FunctionalInterrupt will.  We have a static method that `timerAttachInterrupt()` will take,
	 * but we'd lose instance context.  But the `esp_timer_create_args_t` will let us set a callback
	 * argument, which we set to `this`, so that the static method maintains instance context.
	 * 
	 * As of 29 September 2023, there is an open issue to allow `std::function` in esp32-hal-timer:
	 * https://github.com/espressif/arduino-esp32/issues/8427
	 * 
	 * ...for now (and maybe forever?), we'll do it this way.
	 */

	esp_timer_create_args_t _timerConfig;
	_timerConfig.arg = this;
	_timerConfig.callback = reinterpret_cast<esp_timer_cb_t>( timerCallback );
	_timerConfig.dispatch_method = ESP_TIMER_TASK; 
	_timerConfig.skip_unhandled_events = true;
	_timerConfig.name = PSTR("RotaryEncoder::loop_ISR");

	esp_timer_create( &_timerConfig, &loopTimer );
	esp_timer_start_periodic( loopTimer, RE_LOOP_INTERVAL );
}

void RotaryEncoder::attachInterrupts()
{
	#if defined( BOARD_HAS_PIN_REMAP ) && ( ESP_ARDUINO_VERSION < ESP_ARDUINO_VERSION_VAL(3,0,0) )
		/**
		 * The io_pin_remap.h in Arduino-ESP32 cores of the 2.0.x family
		 * (since 2.0.10) define an `attachInterrupt()` macro that folds-in
		 * a call to `digitalPinToGPIONumber()`, but FunctionalInterrupt.cpp
		 * does this too, so we actually don't need the macro at all.
		 * Since 3.x the call inside the function was removed, so the wrapping
		 * macro is useful again.
		 */
		#undef attachInterrupt
	#endif

	attachInterrupt( encoderPinA, std::bind( &RotaryEncoder::_encoder_ISR, this ), CHANGE );
	attachInterrupt( encoderPinB, std::bind( &RotaryEncoder::_encoder_ISR, this ), CHANGE );

	if( encoderPinButton >= 0 )
		attachInterrupt( encoderPinButton, std::bind( &RotaryEncoder::_button_ISR, this ), RISING );
}

void RotaryEncoder::detachInterrupts()
{
	detachInterrupt( encoderPinA );
	detachInterrupt( encoderPinB );
	detachInterrupt( encoderPinButton );
}

void RotaryEncoder::begin( bool useTimer )
{
	resetEncoderValue();

	encoderChangedFlag = false;
	buttonPressedFlag = false;

	pinMode( encoderPinA, encoderPinMode );
	pinMode( encoderPinB, encoderPinMode );

	if( encoderPinButton > RE_DEFAULT_PIN )
		pinMode( encoderPinButton, buttonPinMode );

	if( encoderPinVcc > RE_DEFAULT_PIN )
	{
		pinMode( encoderPinVcc, OUTPUT );
		digitalWrite( encoderPinVcc, HIGH );
	}

	delay( 20 );
	attachInterrupts();

	if( useTimer )
		beginLoopTimer();
}

bool RotaryEncoder::isEnabled()
{
	return _isEnabled;
}

void RotaryEncoder::enable()
{
	if( _isEnabled )
		return;

	attachInterrupts();

	_isEnabled = true;
}

void RotaryEncoder::disable()
{
	if( !_isEnabled )
		return;

	detachInterrupts();

	_isEnabled = false;
}

bool RotaryEncoder::buttonPressed()
{
	if( !_isEnabled )
		return false;

	bool wasPressed = buttonPressedFlag;

	buttonPressedFlag = false;

	return wasPressed;
}

bool RotaryEncoder::encoderChanged()
{
	if( !_isEnabled )
		return false;

	bool hasChanged = encoderChangedFlag;

	encoderChangedFlag = false;

	return hasChanged;
}

long RotaryEncoder::getEncoderValue()
{
	constrainValue();

	return currentValue;
}

void RotaryEncoder::constrainValue()
{
	if( currentValue < minEncoderValue )
		currentValue = circleValues ? maxEncoderValue : minEncoderValue;

	else if( currentValue > maxEncoderValue )
		currentValue = circleValues ? minEncoderValue : maxEncoderValue;
}

void RotaryEncoder::setEncoderValue( long newValue )
{
	currentValue = newValue;

	constrainValue();
}

void ARDUINO_ISR_ATTR RotaryEncoder::loop()
{
	if( callbackEncoderChanged != NULL && encoderChanged() )
		callbackEncoderChanged( getEncoderValue() );

	if( callbackButtonPressed != NULL && buttonPressed() )
		callbackButtonPressed();
}

void ARDUINO_ISR_ATTR RotaryEncoder::_button_ISR()
{
    static unsigned long _lastInterruptTime = 0;

	if( ( millis() - _lastInterruptTime ) < 30 )
		return;

	buttonPressedFlag = true;

	_lastInterruptTime = millis(); 
}

void ARDUINO_ISR_ATTR RotaryEncoder::_encoder_ISR()
{
	/**
	 * Almost all of this came from a blog post by Garry on GarrysBlog.com:
	 * https://garrysblog.com/2021/03/20/reliably-debouncing-rotary-encoders-with-arduino-and-esp32/
	 * 
	 * Read more about how this works here:
	 * https://www.best-microcontroller-projects.com/rotary-encoder.html
	 */

	static uint8_t _previousAB = 3;
    static int8_t _encoderPosition = 0;
    static unsigned long _lastInterruptTime = 0;
    
	unsigned long _interruptTime = millis();

	bool valueChanged = false;
    
    _previousAB <<=2;  // Remember previous state
    
    if( digitalRead( encoderPinA ) ) _previousAB |= 0x02; // Add current state of pin A
    if( digitalRead( encoderPinB ) ) _previousAB |= 0x01; // Add current state of pin B
    
    _encoderPosition += encoderStates[( _previousAB & 0x0f )];

    // Update counter if encoder has rotated a full detent
	// For the following comments, we'll assume it's 4 steps per detent
	// The tripping point is `STEPS - 1` (so, 3 in this example)

    if( _encoderPosition > encoderTripPoint )               // Four steps forward
    {
        if( _interruptTime - _lastInterruptTime > 40 )      // Greater than 40 milliseconds
            this->currentValue++;                           // Increase by 1
        
        else if( _interruptTime - _lastInterruptTime > 20 ) // Greater than 20 milliseconds
            this->currentValue += 3;                        // Increase by 3

        else                                                // Faster than 20 milliseconds
            this->currentValue += 10;                       // Increase by 10
        
		valueChanged = true;
    }
    else if( _encoderPosition < -encoderTripPoint )         // Four steps backwards
    {
        if( _interruptTime - _lastInterruptTime > 40 )      // Greater than 40 milliseconds
            this->currentValue--;                           // Increase by 1

        else if( _interruptTime - _lastInterruptTime > 20 ) // Greater than 20 milliseconds
            this->currentValue -= 3;                        // Increase by 3

        else                                                // Faster than 20 milliseconds
            this->currentValue -= 10;                       // Increase by 10
        
		valueChanged = true;
    }

	if( valueChanged )
	{
		encoderChangedFlag = true;

        _encoderPosition = 0;
        _lastInterruptTime = millis();                      // Remember time
	}
}