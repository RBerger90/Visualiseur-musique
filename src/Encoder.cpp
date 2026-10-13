#include "Encoder.h"
#include <Arduino.h>

namespace
{
  const int ENCODER_COUNT = 2;

  int clkPin[ENCODER_COUNT];
  int dtPin[ENCODER_COUNT];

  // One step accumulated by the CLK ISR per detent.
  volatile int pendingRawSteps[ENCODER_COUNT];

  // attachInterruptArg passes the encoder index through: no trampoline
  // lambdas needed, each pin's interrupt is unambiguously bound to its
  // own encoder index.
  // Interrupts are attached ONLY to the CLK pin (not DT): half the
  // interrupt load, and a bouncing/floating DT pin can no longer storm
  // the ISR. DT is read as the direction, but ONLY on the FALLING edge of
  // CLK, never CHANGE: the DT-vs-direction relationship flips between
  // CLK's rising and falling edge, so triggering on both (CHANGE) decodes
  // half the transitions backwards. Net effect observed on hardware: a
  // rotation in one consistent direction landed on a preset/parameter
  // change that looked random, because the sum of alternating +1/-1 steps
  // came down to whichever edge the mechanical bounce happened to land on.
  void IRAM_ATTR isrDecode(void *arg)
  {
    int i = (int)(intptr_t)arg;
    int direction = digitalRead(dtPin[i]) ? 1 : -1;
    pendingRawSteps[i] += direction;
  }
}

void encoderSetup(int index, int clk, int dt)
{
  clkPin[index] = clk;
  dtPin[index] = dt;

  pinMode(clk, INPUT_PULLUP);
  pinMode(dt, INPUT_PULLUP);

  attachInterruptArg(digitalPinToInterrupt(clk), isrDecode,
                     (void *)(intptr_t)index, FALLING);
}

int encoderRead(int index)
{
  noInterrupts();
  int rawSteps = pendingRawSteps[index];
  pendingRawSteps[index] = 0;
  interrupts();

  // One raw CLK falling edge per detent now (no more /2); clamped so a
  // glitchy pin (bounce storm between two reads) can never yield an
  // absurd delta in one frame.
  return constrain(rawSteps, -8, 8);
}
