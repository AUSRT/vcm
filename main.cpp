/**
 * @file main.cpp
 * @brief This command file contains the main runtime loop for the VCM as well
 * as the initialisation of other input output devices and routines.
 *
 * @author David Sutton
 *
 * @date 02/06/2024
 */

#include "AnalogIn.h"
#include "AnalogOut.h"
#include "BusOut.h"
#include "DigitalOut.h"
#include "EventFlags.h"
#include "InterfaceDigitalIn.h"
#include "InterruptIn.h"
#include "LowPowerTicker.h"
#include "LowPowerTimeout.h"
#include "PeripheralNames.h"
#include "PinNameAliases.h"
#include "PinNames.h"
#include "PinNamesTypes.h"
#include "PwmOut.h"
#include "ThisThread.h"
#include "config.h"
#include "eventOS_event_timer.h"
#include "mbed.h"
#include "mbed_power_mgmt.h"
#include "pwmout_api.h"
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

// GEAR_BTN is the btn to be used for shifting between FWD and REV.
constexpr auto GEAR_BTN = D4;

// LEFT_IND is the left indicator output.
constexpr auto LEFT_IND = D5;

// TRIGGER is the input for the rear trigger.
constexpr auto TRIGGER = A0;

constexpr auto LED_OUT = D13;

// EMERGENCY_RESET_TIME defines the length of time required to register a btn
// hold for an emergency state reset. (The real time value of this constant is
// computed by the POLLING_FREQ * EMERGENCY_RESET_TIME)
const int EMERGENCY_RESET_TIME = 20;

// START_BTN_HOLD_TIME defines the length of time required to register a btn
// hold for a the START_BTN (The real time value of this constant is
// computed by the POLLING_FREQ * START_BTN_HOLD_TIME)
const int START_BTN_HOLD_TIME = 10;

// GEAR_BTN_HOLD_TIME defines the length of time required to register a btn hold
// for the GEAR_BTN (The real time value of this constant is computed by the
// POLLING_FREQ * START_BTN_HOLD_TIME)
const int GEAR_BTN_HOLD_TIME = 10;

// Used to debounce the gear button.
LowPowerTimeout GEAR_DEBOUNCE_TO;
volatile bool GEAR_DEBOUNCED;

// Define button press interrupt.
InterruptIn off_btn(OFF_BTN, PullUp);
DigitalIn start_btn(START_BTN, PullUp);
InterruptIn gear_btn(GEAR_BTN, PullUp);

// Define analog inputs.
AnalogIn trigger(TRIGGER);
AnalogOut led_out(LED_OUT);

// Setup on board LEDs for writing state.
BusOut onboard_leds(LED1, LED2, LED3);

// Setup indicator light.
PwmOut left_ind(LEFT_IND);

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

// Define a variable which tracks how long the GEAR_BTN has been held for.
volatile int gear_btn_held_count;

// emergency_irq is used to put the car into the OFF state on a single button
// press.
void emergency_irq() {
  // Set the emergency state unless the button has been held.
  if (off_btn_held_count < EMERGENCY_RESET_TIME) {
    flags.set(state::OFF_PRESS);
  }
}

// gear_irq is used to change gears when the GEAR_BTN is pressed.
void gear_irq() {
  if (!GEAR_DEBOUNCED && gear_btn_held_count < GEAR_BTN_HOLD_TIME &&
      gear_btn_held_count != -1) {
    flags.set(state::GEAR_PRESS);
    GEAR_DEBOUNCED = true;
    GEAR_DEBOUNCE_TO.attach([] { GEAR_DEBOUNCED = false; }, 20ms);
  }
}

// TRIGGER_MIN represents the smallest value which the trigger reports.
const float TRIGGER_MIN = 0.252;

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
  if (start_btn_held_count != -1 && !start_btn.read()) {
    start_btn_held_count++;
    if (start_btn_held_count >= START_BTN_HOLD_TIME) {
      flags.set(state::START_HOLD);
      start_btn_held_count = -1;
    }
  } else if (start_btn.read()) {
    start_btn_held_count = 0;
  };

  // Read the GEAR_BTN.
  if (gear_btn_held_count != -1 && !gear_btn.read()) {
    gear_btn_held_count++;
    if (gear_btn_held_count >= GEAR_BTN_HOLD_TIME) {
      flags.set(state::GEAR_HOLD);
      gear_btn_held_count = -1;
    }
  } else if (gear_btn.read()) {
    gear_btn_held_count = 0;
  };
}

void accelerate(float *speed, float rate) {
  if (rate < 0.01) {
    return;
  }
  float _speed = *speed;
  _speed = _speed + pow(0.2 * (rate - 0.01), 1.2);
  if (_speed > 1) {
    *speed = 1;
  } else if (_speed < 0) {
    *speed = 0;
  } else {
    *speed = _speed;
  }
}

// main() runs in its own thread in the OS
int main() {
  //   Poll to check for button holds.
  polling_ticker.attach(&poll_irq, POLLING_FREQ);

  // Set indicators.
  left_ind.period(0.8);
  left_ind.write(0.5);

  //   Set interupt handlers.
  off_btn.rise(&emergency_irq);
  gear_btn.rise(&gear_irq);

  float speed = 0.5;
  float trigger_value;
  float norm_trigger;

  while (true) {
    // Read the event flags to check for a state update. These flags are set in
    // the interrupt handlers.
    if (flags.get() != 0) {
      state_machine.next_state();
      onboard_leds.write(state_machine.get_state());
    }

    float trigger_value = trigger.read();
    float norm_trigger = (trigger_value - TRIGGER_MIN) * 4 / 3;
    accelerate(&speed, norm_trigger);
    led_out.write(speed);
  }
}
