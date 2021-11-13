
int LedPin = 13;

#define BUTTON_DEBUG 1

/*
   Button class. Handles updating buttons.
*/
class Button
{
  public:
    /*
       Button constructor.

       adcPin - analog to digital convertor pin number

       thresh - threshold. If ADC value falls below this, then we say that the
                button is down.

       joystickButton - Which joystick buton to press when this ADC pin is
                pressed.
    */
    Button(int adcPin, int thresh, int joystickButton) :
      adcPin(adcPin), thresh(thresh),
      joystickButton(joystickButton)
    {}

    /*
       Static method to setup things before calling the update methods.
    */
    static void preUpdate()
    {
      led = false;

#ifdef BUTTON_DEBUG
      // Print out the time value to for the first CSV value:
      Serial.print(millis());
#endif
      
    }

    /*
       Update method. Chec the ADC, and press/unpress the button as appropirate.
    */
    void update()
    {
      // Read the ADC:
      int adc = analogRead(adcPin);

#ifdef BUTTON_DEBUG
      // Print CSV data to the serial port, Using Serial.print instead of Serial.printf
      // because it saves considerable space! Program storage went from 23% to 76% when
      // I tried printf.
      Serial.print(adc);
      Serial.print(",");
#endif

      // If the value is below thresh, push the button, otherwise don't.
      Joystick.button(joystickButton, adc < thresh);

      // Update the LED state:
      Button::led |= adc < thresh;
    }

    /*
       Static method for after update stuff.
    */
    static void postUpdate()
    {
      // Actually update the joystick state:
      Joystick.send_now();

      // Update the LED. On if something is pressed, off otherwise:
      digitalWrite(LedPin, led);
#ifdef BUTTON_DEBUG
      // Print out the endline for the serial port CSV data:
      Serial.print("\n");
#endif
    }

  private:
    // ADC pin number:
    int adcPin;

    // Threshold for button up/down:
    int thresh;

    // Joystick button to press when our input button is down:
    int joystickButton;

public:
    static bool led;
};

bool Button::led = false;

// Define the buttons (analog input pin, joystick button, threshold):
//#define PAD_WITH_TOP_CONNECTION 1
#define PAD_WITH_SIDE_CONNECTION

#ifdef PAD_WITH_SIDE_CONNECTION
Button buttons[] = {
  {A0, 200, 1},
  {A2, 200, 2},
  {A3, 200, 3},
  {A5, 200, 4},
  {A6, 200, 5},
  {A8, 200, 6}
};
#elif PAD_WITH_TOP_CONNECTION
Button buttons[] = {
  {A1, 200, 1},
  {A2, 200, 2},
  {A4, 200, 3},
  {A5, 200, 4},
  {A6, 200, 5},
  {A8, 200, 6}
};
#endif

void setup() {
  // put your setup code here, to run once:
  pinMode(LedPin, OUTPUT);

  // Set sendstate to flase. This prevents the Joystick library from updating the state on every
  // set button call. Those calls take like 1ms each, so let's not do that. Make one update so
  // we only pay the price once, not once for every button.
  Joystick.useManualSend(true);
}

// Next time we are due for an update.
unsigned long nextUpdateMs = 1;

#define AFAP

void loop() {

#ifndef AFAP
  // Check the update interval. If not time, loop around again.

  int updateIntervalMs = 1;

  // Get the current time in milliseonds since epoch:
  unsigned long ms = millis();
  if (ms < nextUpdateMs)
    return;

  while (nextUpdateMs <= ms)
    nextUpdateMs += updateIntervalMs;
#endif

  // Update all the buttons:
  Button::preUpdate();

  for (auto button : buttons)
  {
    button.update();
  }
  
  Button::postUpdate();
}
