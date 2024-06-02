#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include "BusOut.h"
#include "EventFlags.h"
#include <cstdint>

// Define the namespace of these values to be state.
namespace state {

/** states enumerates the states of the vehicle. The states are:
 *      OFF:    None of the electronics of the car are active.
 *              We should start in this state.
 *
 *      LV:     The 12v supply rail is on. This powers up all 12V connected
 *              electronics.
 *
 *      HV:     The highvoltage BMS connection in closed. This allows the power
 *              flow to the motor controller.
 */
enum states { OFF, LV, HV };

// Define the event flags which signal which of the following interactions have
// been made.
const std::uint32_t OFF_HOLD(1UL << 1);    // A long press of the OFF_BTN.
const std::uint32_t OFF_PRESS(1UL << 0);   // A single press of the OFF_BTN.
const std::uint32_t START_PRESS(1UL << 2); // A single press of the START_BTN.
const std::uint32_t START_HOLD(1UL << 3);  // A long press of the START_BTN.

} // namespace state

/* StateMachine stores the current state of the vehicle and handles updates to
 * the current state through the event flags. Calls to next_state should be made
 * whenever the event flags are non-nill, these calls should come from the main
 * execution loop.
 */
class StateMachine {
  int _current_state;
  rtos::EventFlags *_flags;

public:
  /* The constructor sets the initial state to OFF, and gets the EventFlags
   * object.
   *
   * @param flags EventFlags object used to pass interactions from an ISR
   * context.
   */
  StateMachine(rtos::EventFlags *flags);

  /* Set the state of the vehicle given an interaction passed via the event
   * flags.
   */
  void next_state();

  /* Get the current state of the vehicle as an enumerated value.
   *
   * @returns
   *    An enumerated value of the current state.
   */
  int get_state();
};

#endif