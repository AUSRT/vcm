#include "BusOut.h"
#include "DigitalOut.h"
#include "InterfaceDigitalIn.h"
#include "InterruptIn.h"
#include "LowPowerTicker.h"
#include "LowPowerTimeout.h"
#include "PinNameAliases.h"
#include "PinNames.h"
#include "PinNamesTypes.h"
#include "ThisThread.h"
#include "mbed.h"
#include <chrono>
#include <cstdio>
#include <ctime>

// POLLING_FREQ is the polling rate to be used in a polling ticker.
constexpr auto POLLING_FREQ = 100ms;
// INPUT_BTN is the btn to be used for inputs.
constexpr auto INPUT_BTN = D2;
// HOLD_TIME defines the length of time required to register a btn hold. (The
// real time value of this constant is computed by the POLLING_FREQ * HOLD_TIME)
const int HOLD_TIME = 50;

// Define button press interrupt.
InterruptIn btn(INPUT_BTN, PullUp);

// Setup status LED
DigitalOut emergency(D3); // Red.

// Setup on board LEDs.
BusOut onboard_leds(LED1, LED2, LED3);

// Define a ticker to poll inputs.
Ticker polling_ticker;

// Define a variable which tracks how long the INPUT_BTN has been held for.
volatile int btn_held_count;

// emergency_irq is used to put the car into the emergency state. This state
// turns off all electronics until the system is rebooted, or the state is
// exited by holding down the button for 5 seconds.
void emergency_irq() {
  // Set the emergency state unless the button has been held.
  if (btn_held_count < HOLD_TIME) {
    emergency = 1;
  }
}

// poll() is used to poll at the POLLING_FREQ for reading different inputs. poll
// can be used to keep track of button holding, and other inputs.
void poll_irq() {
  if (!btn.read()) {
    btn_held_count++;
    if (btn_held_count >= HOLD_TIME) {
      emergency = 0;
    }
  } else {
    btn_held_count = 0;
  };
}

// main() runs in its own thread in the OS
int main() {
  // Poll the button to check for button holds.
  polling_ticker.attach(&poll_irq, POLLING_FREQ);

  // Set interupt handlers.
  btn.rise(&emergency_irq);

  while (true) {
    if (emergency) {
      // Flash LEDs.
      onboard_leds = 0;
      ThisThread::sleep_for(300ms);
      onboard_leds = 7;
      ThisThread::sleep_for(300ms);
    } else {
      // Count up in binary.
      for (int i = 0; i < 8; i++) {
        if (emergency) {
          break;
        }
        onboard_leds = i;
        ThisThread::sleep_for(300ms);
      }
    }
  }
}
