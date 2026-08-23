#include "Button.h"
#include <Arduino.h>

namespace
{
  const unsigned long DEBOUNCE_MS = 30;
  const int MAX_BUTTONS = 4;

  struct ButtonState
  {
    int pin = -1;
    int lastReading = HIGH;
    int stableState = HIGH;
    unsigned long lastChangeTime = 0;
  };

  ButtonState states[MAX_BUTTONS];

  // Un seul pin par bouton suffit ici (pas de multiplexage), donc un petit
  // tableau parcouru lineairement est largement suffisant.
  ButtonState &stateFor(int pin)
  {
    for (int i = 0; i < MAX_BUTTONS; i++)
    {
      if (states[i].pin == pin)
        return states[i];

      if (states[i].pin == -1)
      {
        states[i].pin = pin;
        states[i].lastReading = digitalRead(pin);
        states[i].stableState = states[i].lastReading;
        return states[i];
      }
    }

    return states[MAX_BUTTONS - 1];
  }
}

bool buttonPressed(int pin)
{
  ButtonState &state = stateFor(pin);
  int reading = digitalRead(pin);

  if (reading != state.lastReading)
    state.lastChangeTime = millis();
  state.lastReading = reading;

  if (millis() - state.lastChangeTime > DEBOUNCE_MS && reading != state.stableState)
  {
    state.stableState = reading;
    return state.stableState == LOW;
  }

  return false;
}
