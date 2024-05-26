#include "DigitalOut.h"
#include "InterfaceDigitalIn.h"
#include "InterruptIn.h"
#include "PinNameAliases.h"
#include "PinNames.h"
#include "PinNamesTypes.h"
#include "ThisThread.h"
#include "mbed.h"
#include <cstdio>

// Define button press interrupt.
InterruptIn btn(D2, PullUp);

// Setup status LED
DigitalOut emergency(D3); // Red.

// Create a cooldown for debouncing a button press.
LowPowerTimeout debounce_timeout;
volatile bool btn_rise_cooldown;

void flip() {
  if (!btn_rise_cooldown) {
    // Turn off the led.
    emergency = !emergency;

    // Start a cooldown period.
    btn_rise_cooldown = true;
    debounce_timeout.attach([] { btn_rise_cooldown = false; }, 100ms);
  };
}

// main() runs in its own thread in the OS
int main() {

  // Set interupt handlers.
  btn.rise(&flip);

  while (true) {
  }
}
