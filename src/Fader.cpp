#include "Fader.h"
#include <Arduino.h>

namespace
{
    int faderPin = -1;

    // Raw ADC reads averaged into one faderReadRaw() result. The ESP32 ADC
    // is noisy (a static wiper jitters by tens of units, measured +-50 on
    // the real fader): averaging reduces that jitter. A real wiper move
    // shifts every sample the same way, so it survives the average.
    const int AVERAGE_SAMPLES = 8;
}

void faderSetup(int pin)
{
    faderPin = pin;

    // The fader's voltage divider actively drives the pin: no pull-up is
    // needed (adding one would skew the reading).
    pinMode(faderPin, INPUT);
}

int faderReadRaw()
{
    int sum = 0;
    for (int i = 0; i < AVERAGE_SAMPLES; i++)
        sum += analogRead(faderPin);

    return sum / AVERAGE_SAMPLES;
}
