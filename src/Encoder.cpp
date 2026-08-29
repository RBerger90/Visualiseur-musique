#include "Encoder.h"
#include <Arduino.h>

namespace
{
  int clkPin = -1;
  int dtPin = -1;

  // Current state of the 2 pins, as (CLK << 1 | DT), updated on every
  // interrupt.
  volatile uint8_t state = 0;

  // Accumulates quarter-steps between two mechanical detents (one detent =
  // one full quadrature cycle, see diagram). Reset to zero as soon as a
  // full detent is detected.
  volatile int8_t subSteps = 0;

  // Full detents waiting to be read by encoderRead().
  volatile int pendingDetents = 0;

  // Quadrature transition table, indexed by (previous state << 2 |
  // current state). +1/-1 for a valid step in one direction, 0 for an
  // invalid transition (mechanical bounce or a two-state jump at once).
  const int8_t transitionTable[16] = {
      0, -1, 1, 0,
      1, 0, 0, -1,
      -1, 0, 0, 1,
      0, 1, -1, 0};

  void IRAM_ATTR onPinChange()
  {
    uint8_t newState = (digitalRead(clkPin) << 1) | digitalRead(dtPin);
    uint8_t index = (state << 2) | newState;
    state = newState;

    subSteps += transitionTable[index];

    if (subSteps >= 4)
    {
      pendingDetents++;
      subSteps = 0;
    }
    else if (subSteps <= -4)
    {
      pendingDetents--;
      subSteps = 0;
    }
  }
}

void encoderSetup(int clk, int dt)
{
  clkPin = clk;
  dtPin = dt;

  pinMode(clkPin, INPUT_PULLUP);
  pinMode(dtPin, INPUT_PULLUP);

  state = (digitalRead(clkPin) << 1) | digitalRead(dtPin);

  attachInterrupt(digitalPinToInterrupt(clkPin), onPinChange, CHANGE);
  attachInterrupt(digitalPinToInterrupt(dtPin), onPinChange, CHANGE);
}

int encoderRead()
{
  noInterrupts();
  int detents = pendingDetents;
  pendingDetents = 0;
  interrupts();

  return detents;
}
