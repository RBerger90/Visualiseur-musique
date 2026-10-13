#pragma once

// Supports ENCODER_COUNT encoders, indexed 0 to ENCODER_COUNT-1.

// Call once per encoder in setup(). Configures clkPin/dtPin as inputs with
// pull-up and attaches the interrupts that decode the quadrature signal.
void encoderSetup(int index, int clkPin, int dtPin);

// Number of detents moved since the last call for this encoder: positive
// for clockwise rotation, negative for counter-clockwise, 0 if it hasn't
// moved. Resets the counter on every call, so only poll each index once
// per frame.
int encoderRead(int index);
