#pragma once

// Call once in setup(). Configures pin as an ADC input (the fader wiper).
void faderSetup(int pin);

// Raw 12-bit ADC reading (0-4095) of the fader position, averaged over
// several samples to smooth out ADC noise. Poll it every frame.
int faderReadRaw();
