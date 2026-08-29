#include "Encoder.h"
#include <Arduino.h>

namespace
{
  int clkPin = -1;
  int dtPin = -1;

  // Etat courant des 2 broches, sous forme (CLK << 1 | DT), mis a jour a
  // chaque interruption.
  volatile uint8_t state = 0;

  // Accumule les quarts de piste entre deux crans mecaniques (un cran = un
  // cycle complet de quadrature, cf. schema). Remis a zero des qu'un cran
  // complet est detecte.
  volatile int8_t subSteps = 0;

  // Crans complets en attente de lecture par encoderRead().
  volatile int pendingDetents = 0;

  // Table de transition en quadrature, indexee par (etat precedent << 2 |
  // etat courant). +1/-1 pour un pas valide dans un sens, 0 pour une
  // transition invalide (rebond mecanique ou saut de deux etats a la fois).
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
