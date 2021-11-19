/*
MIT License

Copyright (c) 2021 Brett Elliott

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

const int LedPin = 13;
bool led = false;

#define BUTTON_DEBUG 1
//#define THRESH_DEBUG

#define N_DELAY 16

/*
 * A Delay-line class. Feed samples in, and it comes back out N_DELAY samples later.
 */
struct DelayLine
{
public:
  DelayLine()
  {
    // Initialize the buffer and index to 0.
    for (unsigned int i=0; i<N_DELAY; i++)
      buff[i] = 0;
    idx = 0;
  }

  // Disallow copying and assignment. Prevents accidental oopsies.
  DelayLine(const DelayLine&) = delete;
  DelayLine & operator=(const DelayLine&) = delete;

  // Pop a value off the delay line, and push a new one one:
  unsigned int next(unsigned int n)
  {
    // Grab the next value off the buffer:
    unsigned int ret = buff[idx];

    // Put the new value into the buffer:
    buff[idx] = n;

    // Increment and wrap the index:
    idx++;
    if (idx >= N_DELAY)
      idx = 0;
    
    return ret;
  }
private:
  // Index to read and write next value:
  unsigned int idx;

  // Buffer to hold our circular buffer of data:
  unsigned int buff[N_DELAY];
  
};

struct Thresher
{
public:
  /*
     Thresher constructor.

     absThresh - Static threshold. If the ADC is below this value. Call it on.

     relThresh -  Relative threshold. If we see the signal drop by this much over
                  N_DELAY samples, we also declare the button on. Helps with light
                  taps.
  */
  Thresher(unsigned int absThresh, long relThresh) :
    absThresh(absThresh), relThresh(relThresh)
  {}

  // Disallow copying and assignment. Prevents accidental oopsies.
  Thresher(const Thresher&) = delete;
  Thresher & operator=(const Thresher&) = delete;

  bool next(unsigned int x)
  {
    // Get the same from N_DELAY samples ago, and push the current sample into the
    // delay line:
    long delayed = delayLine.next(x);

    // Convert to signed long, so we can subtract and get a happily signed result:
    long longX = x;

    // Different between same from N_DELAY ago and current sample:
    long diff = delayed - longX;

    // Check if the button is one:
    bool on = false;
    if (x < absThresh)
    {
#ifdef THRESH_DEBUG
      Serial.printf("%lu ABS THRESH ON\n", millis());
#endif
      on = true;
    }
    if (diff > relThresh)
    {
#ifdef THRESH_DEBUG
      Serial.printf("%lu REL THRESH ON\n", millis());
#endif
      on = true;
    }

    return on;
  }

private:

  // Static threshold. If the ADC is below this value. Call it on:
  unsigned int absThresh;
  
  // Realtive threshold. If we see the signal drop by this much over N_DELAY
  // samples, we also declare the button on. Helps with light taps.
  long relThresh;
  DelayLine delayLine;
};

/*
   Button class. Handles updating buttons.
*/
struct Button
{
public:
  /*
     Button constructor.

     adcPin - analog to digital convertor pin number

     thresh - see Thresher

     diffThresh - see Thresher.

     joystickButton - Which joystick buton to press when this ADC pin is
              pressed.
  */
  Button(int adcPin, unsigned int absThresh, long relThresh, int joystickButton) :
    adcPin(adcPin), joystickButton(joystickButton), buttabsThresher(absThresh, relThresh)
  {}

  // Disallow copying and assignment. Prevents accidental oopsies.
  Button(const Button&) = delete;
  Button & operator=(const Button&) = delete;

  /*
     Static method to setup things before calling the update methods.
  */
  static void preUpdate()
  {
    // Turn off the LED:
    led = false;

#ifdef BUTTON_DEBUG
    // Print out the time value to for the first CSV value. Note, Serial.print() 
    // printed the wrong value.
    Serial.printf("%lu,", millis());
#endif
    
  }

  /*
     Update method. Check the ADC, and press/unpress the button as appropirate.
  */
  void update()
  {
    
    // Read the ADC:
    int adc = analogRead(adcPin);

    // Check the thresh, to see if we should turn the button on:
    bool on = buttabsThresher.next(adc);

    // Set the joystick button (doesn't actually update until the postUpdate
    // call).
    Joystick.button(joystickButton, on);

    // Update the LED state:
    led |= on;

#ifdef BUTTON_DEBUG
    // Print the ADC value:
    Serial.printf("%d,", adc);
#endif
  }

  /*
     Static method for after update stuff.
  */
  static void postUpdate()
  {
    // Actually update the joystick state. Sending them all at once is way faster.
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

  // Joystick button to press when our input button is down:
  int joystickButton;

  // Object which does the 
  Thresher buttabsThresher;
};


// I have two pads. One has connections on top, and one on the side. I accidentaly
// wired them up differently, so the have different analog input pins. Oops.
#define PAD_WITH_TOP_CONNECTION 1
//#define PAD_WITH_SIDE_CONNECTION

// Define the buttons (analog input pin, abs thresh, rel thresh, joystick button):
#ifdef PAD_WITH_SIDE_CONNECTION 
Button buttons[] =
{
  {A0, 400, 200, 1},
  {A2, 400, 200, 2},
  {A3, 400, 200, 3},
  {A5, 400, 200, 4},
  {A6, 400, 200, 5},
  {A8, 400, 200, 6}
};
#elif PAD_WITH_TOP_CONNECTION
Button buttons[] =
{
  {A1, 400, 200, 1},
  {A2, 400, 200, 2},
  {A4, 400, 200, 3},
  {A5, 400, 200, 4},
  {A6, 400, 200, 5},
  {A8, 400, 200, 6}
};
#endif

// Arduion setup function
void setup()
{
  // Setup the LedPin to be an output, so we can turn the LED on/off:
  pinMode(LedPin, OUTPUT);

  // Set sendstate to flase. This prevents the Joystick library from updating the state on every
  // set button call. Those calls take like 1ms each, so let's not do that. Make one update so
  // we only pay the price once, not once for every button.
  Joystick.useManualSend(true);
}

void loop()
{
  // Do the pre-udpate step:
  Button::preUpdate();

  // Loop over the buttons, and update each one. Don't forget the &! Without that, instead of
  // a reference, we get a copy of the buttons. Since our buttons need to store state in their
  // delay line, calling update on a copy is no good, since the copy gets updated, instead of
  // the real object. I spent like half a day realizing that. I should make the copy
  // constructors private.
  for (auto& button : buttons)
    button.update();

  // Post-update step:
  Button::postUpdate();
}
