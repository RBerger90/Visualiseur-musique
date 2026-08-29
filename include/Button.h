#pragma once

// Call every frame for a button wired between `pin` and GND (pin set to
// INPUT_PULLUP). Returns true exactly once at the moment of the press
// (falling edge), with software debouncing, even if the button stays held
// down for several frames in a row.
bool buttonPressed(int pin);
