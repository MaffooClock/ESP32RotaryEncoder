#ifndef _RotaryEncoder_h
#define _RotaryEncoder_h

#if defined( ARDUINO ) && ARDUINO >= 100
  #include <Arduino.h>

#elif defined( WIRING )
  #include <Wiring.h>

#else
  #include <WProgram.h>
  #include <pins_arduino.h>

#endif

#if defined( ESP32 )
  #define RE_ISR_ATTR IRAM_ATTR

  #ifdef ARDUINO_ISR_ATTR
    #undef ARDUINO_ISR_ATTR
    #define ARDUINO_ISR_ATTR IRAM_ATTR
  #endif

  #if defined( ESP_ARDUINO_VERSION ) && ( ESP_ARDUINO_VERSION == ESP_ARDUINO_VERSION_VAL(2,0,10) )
    /**
     * BUG ALERT!
     *
     * With Arduino-ESP32 core 2.0.10, the #include statement below
     * fails to compile due to a bug.
     * Also see `attachInterrupts()` in ESP32RotaryEncoder.cpp for
     * the note about the `attachInterrupt()` macro in 2.x cores.
     */
    #error Please upgrade the Arduino-ESP32 core to use this library.
  #else
    #include <FunctionalInterrupt.h>
  #endif
#endif

#define RE_DEFAULT_PIN  -1
#define RE_DEFAULT_STEPS 4
#define RE_LOOP_INTERVAL 100000U  // 0.1 seconds

typedef enum {
  FLOATING,
  HAS_PULLUP,
  SW_FLOAT
} EncoderType;

class RotaryEncoder {

  protected:
    mutable portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

    #if defined( ESP32 )
      typedef std::function<void(long)> EncoderCallback;
      typedef std::function<void(unsigned long)> ButtonCallback;
    #else
      typedef void (*EncoderCallback)(long);
      typedef void (*ButtonCallback)(unsigned long);
    #endif


  public:

    /**
     * @brief Construct a new Rotary Encoder instance
     *
     * @param encoderPinA        The A pin on the encoder, sometimes marked "CLK"
     * @param encoderPinB        The B pin on the encoder, sometimes marked "DT"
     * @param encoderPinButton   Optional; the pushbutton pin, could be marked "SW"
     * @param encoderPinVcc      Optional; the voltage reference input, could be marked "+" or "V+" or "VCC"; defaults to -1, which is ignored
     * @param encoderSteps       Optional; the number of steps per detent; usually 4 (default), could be 2
     */
    RotaryEncoder(
      uint8_t encoderPinA,
      uint8_t encoderPinB,
      int8_t encoderPinButton = RE_DEFAULT_PIN,
      int8_t encoderPinVcc = RE_DEFAULT_PIN,
      uint8_t encoderSteps = RE_DEFAULT_STEPS
    );

    /**
     * @brief Responsible for detaching interrupts and clearing the loop timer
     *
     */
    ~RotaryEncoder();

    /**
     * @brief Specifies whether the encoder pins need to use the internal pull-up resistors.
     *
     * @note Call this in `setup()`.
     *
     * @param type  FLOATING if you're using a raw encoder not mounted to a PCB (internal pull-ups will be used);
     *              HAS_PULLUP if your encoder is a module that has pull-up resistors, (internal pull-ups will not be used);
     *              SW_FLOAT your encoder is a module that has pull-up resistors, but the resistor for the switch is missing (internal pull-up will be used for switch input only)
     */
    void setEncoderType( EncoderType type );

    /**
     * @brief Set the minimum and maximum values that the encoder will return.
     *
     * @note This is a convenience function that calls `setMinValue()`, `setMaxValue()`, and `setCircular()`
     *
     * @param minValue      Minimum value (e.g. 0)
     * @param maxValue      Maximum value (e.g. 10)
     * @param circleValues  If true, turning past the maximum will wrap around to the minimum and vice-versa
     *                      If false (default), turning past the minimum or maximum will return that boundary
     */
    void setBoundaries( long minValue, long maxValue, bool circleValues = false );

    /**
     * @brief Set the minimum value that the encoder will return.
     *
     * @note Call this in `setup()`
     *
     * @param minValue  Minimum value
     */
    void setMinValue( long minValue );

    /**
     * @brief Set the maximum value that the encoder will return.
     *
     * @note Call this in `setup()`
     *
     * @param maxValue  Maximum value
     */
    void setMaxValue( long maxValue );

    /**
     * @brief Set whether the minimum or maximum value will wrap around to the other.
     *
     * @note Call this in `setup()`
     *
     * @param maxValue  Maximum value
     */
    void setCircular( bool circleValues );

    /**
     * @brief Set the amount of increment/decrement by which the value tracked by the encoder will change.
     *
     * @note Call this in `setup()`
     *
     * @param stepValue  Step value
     */
    void setStepValue( long stepValue );

    /**
     * @brief Set a function to fire every time the value tracked by the encoder changes.
     *
     * @note Call this in `setup()`.  May be set/changed at runtime if needed.
     *
     * @param handler The function to call; it must accept one parameter
     *                of type long, which will be the current value
     */
    void onTurned( EncoderCallback f );

    /**
     * @brief Set a function to fire every time the the pushbutton is pressed.
     *
     * @note Call this in `setup()`.  May be set/changed at runtime if needed.
     *
     * @param handler The function to call; it must accept one parameter of type long, which
     *                will be the duration (in milliseconds) that the button was active
     */
    void onPressed( ButtonCallback f );

    /**
     * @brief Sets up the GPIO pins specified in the constructor and attaches the ISR callback for the encoder.
     *
     * @note Call this in `setup()` after other "set" methods.
     *
     */
    void begin( bool useTimer = true );

    /**
     * @brief Enables the encoder knob and pushbutton if `disable()` was previously used.
     *
     */
    void enable();

    /**
     * @brief Disables the encoder knob and pushbutton.
     *
     * Knob rotation and button presses will have no effect until after `enable()` is called
     *
     */
    void disable();

    /**
     * @brief Confirms whether the encoder knob and pushbutton have been disabled.
     *
     */
    bool isEnabled();

    /**
     * @brief Check if the pushbutton has been pressed.
     *
     * @note Call this in `loop()` to fire a handler.
     *
     * @return true if the button was pressed since the last time it was checked,
     *         false if the button has not been pressed since the last time it was checked
     */
    bool buttonPressed();

    /**
     * @brief Check if the value tracked by the encoder has changed.
     *
     * @note Call this in `loop()` to fire a handler for the new value.
     *
     * @return true if the value is different than the last time it was checked,
     *         false if the value is the same as the last time it was checked
     */
    bool encoderChanged();

    /**
     * @brief Get the current value tracked by the encoder.
     *
     * @return A value between the minimum and maximum configured by `setBoundaries()`
     */
    long getEncoderValue();

    /**
     * @brief Override the value tracked by the encoder.
     *
     * @note If the new value is outside the minimum or maximum configured
     * by `setBoundaries()`, it will be adjusted accordingly
     *
     * @param newValue
     */
    void setEncoderValue( long newValue );

    /**
     * @brief Reset the value tracked by the encoder.
     *
     * @note This will try to set the value to 0, but if the minimum and maximum configured
     * by `setBoundaries()` does not include 0, then the minimum or maximum will be
     * used instead
     *
     */
    void resetEncoderValue() { setEncoderValue( 0 ); }

    /**
     * @brief Synchronizes the encoder value and button state from ISRs.
     *
     * Runs on a timer and calls `encoderChanged()` and `buttonPressed()` to determine
     * if user-specified callbacks should be run.
     *
     * This would normally be called in userspace `loop()`, but we're using the `loopTimer` instead.
     *
     */
    void ARDUINO_ISR_ATTR loop();

  private:

    const char *LOG_TAG = "ESP32RotaryEncoder";

    EncoderCallback callbackEncoderChanged = NULL;
    ButtonCallback callbackButtonPressed = NULL;

    typedef enum {
        LEFT  = -1,
        STILL =  0,
        RIGHT =  1
    } Rotation;

    Rotation encoderStates[16] = {
      STILL, LEFT,  RIGHT, STILL,
      RIGHT, STILL, STILL, LEFT,
      LEFT,  STILL, STILL, RIGHT,
      STILL, RIGHT, LEFT,  STILL
    };

    int encoderPinMode = INPUT;
    int buttonPinMode  = INPUT;

    uint8_t encoderPinA;
    uint8_t encoderPinB;
    int8_t encoderPinButton;
    int8_t encoderPinVcc;
    uint8_t encoderTripPoint;

    /**
     * @brief Determines whether knob turns or button presses will be ignored.  ISRs still fire,
     *
     * Set by `enable()` and `disable()`.
     *
     */
    bool _isEnabled = true;

    /**
     * @brief Sets the minimum and maximum values of `currentValue`.
     *
     * Set in `setBoundaries()` and used in `constrainValue()`.
     *
     */
    long minEncoderValue = -1; long maxEncoderValue = 1;

    /**
     * @brief The amount of increment/decrement that will be applied to the
     * encoder value when the knob is turned.
     *
     */
    long stepValue = 1;

    /**
     * @brief Determines whether attempts to increment or decrement beyond
     * the boundaries causes `currentValue` to wrap to the other boundary.
     *
     * Set in `setBoundaries()`.
     *
     */
    bool circleValues = false;

    /**
     * @brief The value tracked by `encoder_ISR()` when the encoder knob is turned.
     *
     * This value can be overwritten in `constrainValue()` whenever
     * `getEncoderValue()` or `setEncoderValue()` are called.
     *
     */
    volatile long currentValue;

    /**
     * @brief Becomes `true` when `encoder_ISR()` changes `currentValue`,
     * then becomes `false` when caught by `loop()` via `encoderChanged()`
     *
     */
    volatile bool encoderChangedFlag;

    /**
     * @brief Becomes `true` when `button_ISR()` changes `currentValue`,
     * then becomes `false` when caught by `loop()` via `buttonPressed()`
     *
     */
    volatile bool buttonPressedFlag;

    /**
     * @brief
     *
     */
    volatile unsigned long buttonPressedTime, buttonPressedDuration;

    /**
     * @brief The loop timer configured and started in `beginLoopTimer()`.
     *
     * This replaces the need to run the class loop in userspace `loop()`.
     *
     */
    esp_timer_handle_t loopTimer;

    /**
     * @brief Constrains the value set by `encoder_ISR()` and `setEncoderValue()`
     * to be in the range set by `setBoundaries()`.
     *
     */
    void constrainValue();

    /**
     * @brief Attaches ISRs to encoder and button pins.
     *
     * Used in `begin()` and `enable()`.
     *
     */
    void attachInterrupts();

    /**
     * @brief Detaches ISRs from encoder and button pins.
     *
     * Used in the destructor and in `disable()`.
     *
     */
    void detachInterrupts();

    /**
     * @brief Sets up the loop timer and starts it.
     *
     * Called in `begin()`.
     *
     */
    void beginLoopTimer();

    /**
     * @brief Static method called by the loop timer, which calls the loop function on a given instance.
     *
     * We need this because the timer cannot call a class method directly, but it can
     * call a static method with an argument, so this gets around that limitation.
     *
     * @param arg
     */
    static void ARDUINO_ISR_ATTR timerCallback( void *arg )
    {
      RotaryEncoder *instance = (RotaryEncoder *)arg;
      instance->loop();
    }

    /**
     * @brief Interrupt Service Routine for the encoder.
     *
     * Detects direction of knob turn and increments/decrements `currentValue`, and sets
     * the `encoderChangedFlag` to be picked up by `encoderChanged()` in `_loop()`.
     *
     */
    void ARDUINO_ISR_ATTR _encoder_ISR();

    /**
     * @brief Interrupt Service Routine for the pushbutton.
     *
     * Sets the `buttonPressedFlag` to be picked up by `buttonPressed()` in `_loop()`.
     *
     */
    void ARDUINO_ISR_ATTR _button_ISR();
};

#endif
