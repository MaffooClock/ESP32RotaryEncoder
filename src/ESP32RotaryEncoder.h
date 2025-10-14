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

#include <atomic>

static constexpr int8_t RE_DEFAULT_PIN = -1;
static constexpr uint8_t RE_DEFAULT_STEPS = 4;
static constexpr uint64_t RE_LOOP_INTERVAL = 100000U;  // 0.1 seconds

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
     * @param buttonPin          Optional; the pushbutton pin, could be marked "SW"
     * @param vccPin             Optional; the voltage reference input, could be marked "+" or "V+" or "VCC"; defaults to -1, which is ignored
     * @param encoderSteps       Optional; the number of steps per detent; usually 4 (default), could be 2. A value of 0 will be imply the default.
     */
    RotaryEncoder(
      uint8_t encoderPinA,
      uint8_t encoderPinB,
      int8_t buttonPin = RE_DEFAULT_PIN,
      int8_t vccPin = RE_DEFAULT_PIN,
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
    void loop();

  private:
    EncoderCallback callbackEncoderChanged = nullptr;
    ButtonCallback callbackButtonPressed = nullptr;

    int encoderPinMode = INPUT;
    int buttonPinMode  = INPUT;

    const uint8_t encoderPinA;
    const uint8_t encoderPinB;
    const int8_t buttonPin;
    const int8_t vccPin;
    const uint8_t encoderSteps;

    std::atomic<bool> _isEnabled{true};

    long minEncoderValue = -1;
    long maxEncoderValue = 1;
    bool circleValues = false;
    long stepValue = 1;

    long currentValue;
    unsigned long _lastRotaryInterruptTime;
    uint8_t _previousAB;
    int8_t _encoderPosition;
    bool encoderChangedFlag;

    long constrainValue( long value ) const;

    bool buttonPressedFlag;
    unsigned long _lastButtonInterruptTime;
    unsigned long buttonPressedTime;
    unsigned long buttonPressedDuration;

    esp_timer_handle_t loopTimer;
    void beginLoopTimer();

    void attachInterrupts();
    void detachInterrupts();

    void _encoder_ISR();
    void _button_ISR();
};

#endif
