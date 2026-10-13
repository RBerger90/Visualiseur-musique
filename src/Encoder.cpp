#include "Encoder.h"
#include <Arduino.h>

namespace
{
  const int ENCODER_COUNT = 2;

  int clkPin[ENCODER_COUNT];
  int dtPin[ENCODER_COUNT];

  // Current state of the 2 pins, as (CLK << 1 | DT), updated on every
  // interrupt.
  volatile uint8_t state[ENCODER_COUNT];

  // Accumulates quarter-steps between two mechanical detents (one detent =
  // one full quadrature cycle, see diagram). Reset to zero as soon as a
  // full detent is detected.
  volatile int8_t subSteps[ENCODER_COUNT];

  // Full detents waiting to be read by encoderRead().
  volatile int pendingDetents[ENCODER_COUNT];

  // Quadrature transition table, indexed by (previous state << 2 |
  // current state). +1/-1 for a valid step in one direction, 0 for an
  // invalid transition (mechanical bounce or a two-state jump at once).
  const int8_t transitionTable[16] = {
      0, -1, 1, 0,
      1, 0, 0, -1,
      -1, 0, 0, 1,
      0, 1, -1, 0};

  // One trampoline per encoder: attachInterrupt() can't pass arguments,
  // and a shared global index would be wrong (both encoders' interrupts
  // fire after both setup() calls, so a global set at setup time would
  // always hold the last index).
  void IRAM_ATTR isrDecode(int i)
  {
    uint8_t newState = (digitalRead(clkPin[i]) << 1) | digitalRead(dtPin[i]);
    uint8_t index = (state[i] << 2) | newState;
    state[i] = newState;

    subSteps[i] += transitionTable[index];

    if (subSteps[i] >= 4)
    {
      pendingDetents[i]++;
      subSteps[i] = 0;
    }
    else if (subSteps[i] <= -4)
    {
      pendingDetents[i]--;
      subSteps[i] = 0;
    }
  }
}

void encoderSetup(int index, int clk, int dt)
{
  clkPin[index] = clk;
  dtPin[index] = dt;

  pinMode(clk, INPUT_PULLUP);
  pinMode(dt, INPUT_PULLUP);

  state[index] = (digitalRead(clk) << 1) | digitalRead(dt);

  attachInterrupt(digitalPinToInterrupt(clk), []
                  { isrDecode(index); }, CHANGE);
  attachInterrupt(digitalPinToInterrupt(dt), []
                  { isrDecode(index); }, CHANGE);
}

int encoderRead(int index)
{
  noInterrupts();
  int detents = pendingDetents[index];
  pendingDetents[index] = 0;
  interrupts();

  return detents;
}
