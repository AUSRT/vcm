#include "DigitalOut.h"
#include "InterfaceDigitalIn.h"
#include "InterruptIn.h"
#include "PinNameAliases.h"
#include "PinNames.h"
#include "PinNamesTypes.h"
#include "ThisThread.h"
#include "mbed.h"
#include <cstdio>

// Setup gpio.
InterruptIn btn(D2, PullDown);
DigitalOut led(LED1);
DigitalOut flash(LED2);

// flip() flips the output of the LED from on to off
void flip() {
    led = !led;
    return;
}

// main() runs in its own thread in the OS
int main()
{    
    btn.rise(&flip);
    while (true) {
        flash = !flash;
        ThisThread::sleep_for(500ms);
    }
}

