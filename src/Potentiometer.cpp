#include "Potentiometer.h"
#include <Arduino.h>

namespace
{
  // Poids de la nouvelle lecture dans la moyenne mobile (plus petit = plus
  // lisse mais plus lent a suivre un vrai mouvement du potar).
  const float SMOOTHING = 0.1;

  // Variation (sur 0-4095) entre deux lectures lissees successives au-dela de
  // laquelle on considere que le potar est reellement en train de bouger,
  // par opposition au bruit residuel de l'ADC.
  const float NOISE_THRESHOLD = 8;

  // Duree sans mouvement detecte avant de figer la valeur renvoyee.
  const unsigned long LOCK_DELAY_MS = 200;

  float filteredValue = 0;
  float prevFilteredValue = 0;
  int stableValue = 0;
  unsigned long lastMoveTime = 0;
  bool locked = false;
}

int potRead(const int potPin, const int minValue, const int maxValue)
{
  int rawValue = analogRead(potPin);
  filteredValue += (rawValue - filteredValue) * SMOOTHING;

  // Un ecart significatif entre deux lectures lissees = le potar bouge :
  // on redemarre le delai avant figeage et on suit la valeur en direct.
  if (abs(filteredValue - prevFilteredValue) > NOISE_THRESHOLD)
  {
    lastMoveTime = millis();
    locked = false;
  }
  prevFilteredValue = filteredValue;

  if (!locked)
  {
    stableValue = (int)filteredValue;
    if (millis() - lastMoveTime > LOCK_DELAY_MS)
      locked = true;
  }

  return map(stableValue, 0, 4095, minValue, maxValue);
}
