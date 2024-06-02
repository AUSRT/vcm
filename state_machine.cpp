#include "state_machine.h"
#include "BusOut.h"
#include "EventFlags.h"

StateMachine::StateMachine(rtos::EventFlags *flags)
    : _current_state(state::OFF), _flags(flags) {}

void StateMachine::next_state() {
  // Get the interaction from the event flag, then clear the flags.
  uint32_t interaction = _flags->get();
  _flags->clear();

  switch (_current_state) {
  case state::OFF: // The car is currrently powered off.
    switch (interaction) {
    case state::OFF_HOLD: // The OFF_BTN has been held, ready to turn on low
                          // voltage.
      _current_state = state::LV;
      break;
    }
    break;
  case state::LV:
    switch (interaction) {
    case state::OFF_PRESS:
      _current_state = state::OFF;
      break;
    case state::START_HOLD:
      _current_state = state::HV;
      break;
    }
    break;
  case state::HV:
    switch (interaction) {
    case state::OFF_PRESS:
      _current_state = state::OFF;
      break;
    case state::START_HOLD:
      _current_state = state::LV;
      break;
    }
    break;
  default:
    _current_state = state::OFF;
  }
}

int StateMachine::get_state() { return _current_state; }