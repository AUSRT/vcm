#include "BusOut.h"
#include "DigitalOut.h"
#include "EventFlags.h"
#include "InterfaceDigitalIn.h"
#include "InterruptIn.h"
#include "LowPowerTicker.h"
#include "LowPowerTimeout.h"
#include "PinNameAliases.h"
#include "PinNames.h"
#include "PinNamesTypes.h"
#include "ThisThread.h"
#include "config.h"
#include "mbed.h"
#include "state_machine.h"
#include <chrono>
#include <cstdio>
#include <ctime>

// POLLING_FREQ is the polling rate to be used in a polling ticker.
constexpr auto POLLING_FREQ = 100ms;

// OFF_BTN is the btn to be used for emergency off, and powering up.
constexpr auto OFF_BTN = D2;

// START_BTN is the btn to be used for changing states form LV to HV.
constexpr auto START_BTN = D3;

// EMERGENCY_RESET_TIME defines the length of time required to register a btn
// hold for an emergency state reset. (The real time value of this constant is
// computed by the POLLING_FREQ * EMERGENCY_RESET_TIME)
const int EMERGENCY_RESET_TIME = 50;

// START_BTN_HOLD_TIME defines the length of time required to register a btn
// hold for a the START_BTN (The real time value of this constant is
// computed by the POLLING_FREQ * START_BTN_HOLD_TIME)
const int START_BTN_HOLD_TIME = 20;

// Define button press interrupt.
InterruptIn off_btn(OFF_BTN, PullUp);
InterruptIn start_btn(START_BTN, PullUp);

// Setup on board LEDs for writing state.
BusOut onboard_leds(LED1, LED2, LED3);

// Define a ticker to poll inputs.
Ticker polling_ticker;

// Define an event flag to manage state changes from ISR.
EventFlags flags;

// Create a state machine which handles the state of the car.
StateMachine state_machine = StateMachine(&flags);

// Define a variable which tracks how long the OFF_BTN has been held for.
volatile int off_btn_held_count;

// Define a variable which tracks how long the START_BTN has been held for.
volatile int start_btn_held_count;

// emergency_irq is used to put the car into the OFF state on a single button
// press.
void emergency_irq() {
  // Set the emergency state unless the button has been held.
  if (off_btn_held_count < EMERGENCY_RESET_TIME) {
    flags.set(state::OFF_PRESS);
  }
}

// poll() is used to poll at the POLLING_FREQ for reading different inputs. poll
// can be used to keep track of button holding, and other inputs.
void poll_irq() {
  // Read the OFF_BTN.
  if (!off_btn.read()) {
    off_btn_held_count++;
    if (off_btn_held_count >= EMERGENCY_RESET_TIME) {
      flags.set(state::OFF_HOLD);
    }
  } else {
    off_btn_held_count = 0;
  };

  // Read the START_BTN.
  if (!start_btn.read()) {
    start_btn_held_count++;
    if (start_btn_held_count >= START_BTN_HOLD_TIME) {
      flags.set(state::START_HOLD);
      start_btn_held_count = 0;
    }
  } else {
    start_btn_held_count = 0;
  };
}

// main() runs in its own thread in the OS
int main() {
  //   Poll to check for button holds.
  polling_ticker.attach(&poll_irq, POLLING_FREQ);

  //   Set interupt handlers.
  off_btn.rise(&emergency_irq);

  while (true) {
    // Read the event flags to check for a state update. These flags are set in
    // the interrupt handlers.
    if (flags.get() != 0) {
      state_machine.next_state();
      onboard_leds.write(state_machine.get_state());
    }
  }
}
