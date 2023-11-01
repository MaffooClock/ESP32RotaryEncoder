#include "ESP32RotaryEncoder.h"

RotaryEncoder::RotaryEncoder( uint8_t encoderPinA, uint8_t encoderPinB, int8_t encoderPinButton, int8_t encoderPinVcc, uint8_t encoderSteps )
{
  this->encoderPinA      = encoderPinA;
  this->encoderPinB      = encoderPinB;
  this->encoderPinButton = encoderPinButton;
  this->encoderPinVcc    = encoderPinVcc;
  this->encoderTripPoint = encoderSteps - 1;

  ESP_LOGD( LOG_TAG, "Initialized: A = %u, B = %u, Button = %i, VCC = %i, Steps = %u", encoderPinA, encoderPinB, encoderPinButton, encoderPinVcc, encoderSteps );
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

    default:
      ESP_LOGE( LOG_TAG, "Invalid encoder type %i", type );
      return;
  }

  ESP_LOGD( LOG_TAG, "Encoder type set to %i", type );
}

void RotaryEncoder::setBoundaries( long minValue, long maxValue, bool circleValues )
{
  if( minValue > maxValue )
    ESP_LOGW( LOG_TAG, "Minimum value (%ld) is greater than maximum value (%ld); behavior is undefined.", minValue, maxValue );

  setMinValue( minValue );
  setMaxValue( maxValue );
  setCircular( circleValues );
}

void RotaryEncoder::setMinValue( long minValue )
{
  ESP_LOGD( LOG_TAG, "minValue = %ld", minValue );

  this->minEncoderValue = minValue;
}

void RotaryEncoder::setMaxValue( long maxValue )
{
  ESP_LOGD( LOG_TAG, "maxValue = %ld", maxValue );

  this->maxEncoderValue = maxValue;
}

void RotaryEncoder::setCircular( bool circleValues )
{
  ESP_LOGD( LOG_TAG, "Boundaries %s circular", ( circleValues ? "are" : "are not" ) );

  this->circleValues = circleValues;
}

void RotaryEncoder::setStepValue( long stepValue )
{
  ESP_LOGD( LOG_TAG, "stepValue = %ld", stepValue );

  if( stepValue > maxEncoderValue || stepValue < minEncoderValue )
    ESP_LOGW( LOG_TAG, "Step value (%ld) is outside the bounds (%ld...%ld); behavior is undefined.", stepValue, minEncoderValue, maxEncoderValue );

  this->stepValue = stepValue;
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
  _timerConfig.name = "RotaryEncoder::loop_ISR";

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

  if( encoderPinButton > RE_DEFAULT_PIN )
    attachInterrupt( encoderPinButton, std::bind( &RotaryEncoder::_button_ISR, this ), CHANGE );

  ESP_LOGD( LOG_TAG, "Interrupts attached" );
}

void RotaryEncoder::detachInterrupts()
{
  detachInterrupt( encoderPinA );
  detachInterrupt( encoderPinB );
  detachInterrupt( encoderPinButton );

  ESP_LOGD( LOG_TAG, "Interrupts detached" );
}

void RotaryEncoder::begin( bool useTimer )
{
  resetEncoderValue();

  encoderChangedFlag = false;
  buttonPressedFlag = false;
  buttonPressedTime = 0;
  buttonPressedDuration = 0;

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

  ESP_LOGD( LOG_TAG, "RotaryEncoder active" );
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

  ESP_LOGD( LOG_TAG, "Input enabled" );
}

void RotaryEncoder::disable()
{
  if( !_isEnabled )
    return;

  detachInterrupts();

  _isEnabled = false;

  ESP_LOGD( LOG_TAG, "Input disabled" );
}

bool RotaryEncoder::buttonPressed()
{
  if( !_isEnabled )
    return false;

  if( buttonPressedFlag )
    ESP_LOGD( LOG_TAG, "Button pressed for %lu ms", buttonPressedDuration );

  bool wasPressed = buttonPressedFlag;

  buttonPressedFlag = false;

  return wasPressed;
}

bool RotaryEncoder::encoderChanged()
{
  if( !_isEnabled )
    return false;

  if( encoderChangedFlag )
    ESP_LOGD( LOG_TAG, "Knob turned; value: %ld", getEncoderValue() );

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
  long unconstrainedValue = currentValue;

  if( currentValue < minEncoderValue )
    currentValue = circleValues ? maxEncoderValue : minEncoderValue;

  else if( currentValue > maxEncoderValue )
    currentValue = circleValues ? minEncoderValue : maxEncoderValue;

  if( unconstrainedValue != currentValue )
    ESP_LOGD( LOG_TAG, "Encoder value '%ld' constrained to '%ld'", unconstrainedValue, currentValue );
}

void RotaryEncoder::setEncoderValue( long newValue )
{
  if( newValue != currentValue )
    ESP_LOGD( LOG_TAG, "Overriding encoder value from '%ld' to '%ld'", currentValue, newValue );

  currentValue = newValue;

  constrainValue();
}

void ARDUINO_ISR_ATTR RotaryEncoder::loop()
{
  if( callbackEncoderChanged != NULL && encoderChanged() )
    callbackEncoderChanged( getEncoderValue() );

  if( callbackButtonPressed != NULL && buttonPressed() )
    callbackButtonPressed( buttonPressedDuration );
}

void ARDUINO_ISR_ATTR RotaryEncoder::_button_ISR()
{
  static unsigned long _lastInterruptTime = 0;

  // Simple software de-bounce
  if( ( millis() - _lastInterruptTime ) < 30 )
    return;

  // HIGH = idle, LOW = active
  bool isPressed = !digitalRead( encoderPinButton );

  if( isPressed )
  {
    buttonPressedTime = millis();

    ESP_EARLY_LOGV( LOG_TAG, "Button pressed at %u", buttonPressedTime );
  }
  else
  {
    unsigned long now = millis();
    buttonPressedDuration = now - buttonPressedTime;

    ESP_EARLY_LOGV( LOG_TAG, "Button released at %u", now );

    buttonPressedFlag = true;
  }

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
  static long _stepValue;

  bool valueChanged = false;

  _previousAB <<=2;  // Remember previous state

  if( digitalRead( encoderPinA ) ) _previousAB |= 0x02; // Add current state of pin A
  if( digitalRead( encoderPinB ) ) _previousAB |= 0x01; // Add current state of pin B

  _encoderPosition += encoderStates[( _previousAB & 0x0f )];


  /**
   * Based on how fast the encoder is being turned, we can apply an acceleration factor
   */

  unsigned long speed = millis() - _lastInterruptTime;

  if( speed > 40 )                                 // Greater than 40 milliseconds
    _stepValue = this->stepValue;                  // Increase/decrease by 1 x stepValue

  else if( speed > 20 )                            // Greater than 20 milliseconds
    _stepValue = ( this->stepValue <= 9 ) ?        // Increase/decrease by 3 x stepValue
      this->stepValue : ( this->stepValue * 3 )    // But only if stepValue > 9
    ;

  else                                             // Faster than 20 milliseconds
    _stepValue = ( this->stepValue <= 100 ) ?      // Increase/decrease by 10 x stepValue
      this->stepValue : ( this->stepValue * 10 )   // But only if stepValue > 100
    ;


  /**
   * Update counter if encoder has rotated a full detent
   * For the following comments, we'll assume it's 4 steps per detent
   * The tripping point is `STEPS - 1` (so, 3 in this example)
   */

  if( _encoderPosition > encoderTripPoint )        // Four steps forward
  {
    this->currentValue += _stepValue;
    valueChanged = true;
  }
  else if( _encoderPosition < -encoderTripPoint )  // Four steps backwards
  {
    this->currentValue -= _stepValue;
    valueChanged = true;
  }

  if( valueChanged )
  {
    encoderChangedFlag = true;

    // Reset our "step counter"
    _encoderPosition = 0;

    // Remember current time so we can calculate speed
    _lastInterruptTime = millis();
  }
}
