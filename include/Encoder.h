#pragma once

// Call once in setup(). Configures clkPin/dtPin as inputs with pull-up and
// attaches the interrupts that decode the quadrature signal.
void encoderSetup(int clkPin, int dtPin);

// Number of detents moved since the last call: positive for clockwise
// rotation, negative for counter-clockwise, 0 if the encoder hasn't moved.
// Resets the counter on every call, so only poll it once per frame.
int encoderRead();
