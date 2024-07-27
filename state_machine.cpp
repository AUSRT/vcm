#include "state_machine.h"
#include "BusOut.h"
#include "EventFlags.h"

StateMachine::StateMachine(rtos::EventFlags *flags)
    : _current_state(state::OFF), _flags(flags) {}

void StateMachine::next_state() {
  // Get the interaction from the event flag, then clear the flags.
  uint32_t interaction = _flags->get();
  _flags->clear();

  // Check the emergency flag first.
  if ((interaction & state::OFF_PRESS) != 0) {
    _current_state = state::OFF;
    return;
  }

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
    case state::START_HOLD:
      _current_state = state::HV;
      break;
    }
    break;
  case state::HV:
    switch (interaction) {
    case state::START_HOLD:
      _current_state = state::LV;
      break;
    case state::GEAR_PRESS:
      _current_state = state::FWD;
      break;
    case state::GEAR_HOLD:
      _current_state = state::REV;
      break;
    }
    break;
  case state::FWD:
    switch (interaction) {
    case state::GEAR_PRESS:
    case state::GEAR_HOLD:
      _current_state = state::HV;
      break;
    }
    break;
  case state::REV:
    switch (interaction) {
    case state::GEAR_PRESS:
    case state::GEAR_HOLD:
      _current_state = state::HV;
      break;
    }
    break;
  default:
    _current_state = state::OFF;
  }
}

int StateMachine::get_state() { return _current_state; }