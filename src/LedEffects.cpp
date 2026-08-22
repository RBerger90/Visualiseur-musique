#include "LedEffects.h"

namespace
{
  const int NUM_LEDS = MATRIX_WIDTH * MATRIX_HEIGHT;
  const int LED_DATA_PIN = 18;

  CRGB leds[NUM_LEDS];

  const CRGB columnColors[MATRIX_WIDTH] = {
      CRGB::Red,
      CRGB::Orange,
      CRGB::Yellow,
      CRGB::Green,
      CRGB::Blue,
      CRGB::Indigo,
      CRGB::Violet,
      CRGB::Pink,
      CRGB::Cyan,
      CRGB::Lime,
      CRGB::Magenta,
      CRGB::Teal,
      CRGB::Maroon,
      CRGB::Navy,
      CRGB::Olive,
      CRGB::Purple,
      CRGB::Silver,
      CRGB::Gold,
      CRGB::Coral,
      CRGB::Salmon,
      CRGB::Turquoise,
      CRGB::Lavender,
      CRGB::Chocolate,
      CRGB::Crimson,
  };

  // Le cablage serpente d'une colonne a l'autre : les lignes paires se lisent
  // de gauche a droite, les lignes impaires de droite a gauche.
  int ledIndex(int x, int y)
  {
    if (y % 2 == 0)
      return y * MATRIX_WIDTH + x;
    return y * MATRIX_WIDTH + (MATRIX_WIDTH - 1 - x);
  }

  void setColumn(int x, int height, CRGB color)
  {
    height = constrain(height, 0, MATRIX_HEIGHT);

    for (int y = 0; y < MATRIX_HEIGHT; y++)
    {
      leds[ledIndex(x, y)] = (y < height) ? color : CRGB::Black;
    }
  }
}

void ledEffectsSetup()
{
  FastLED.addLeds<WS2811, LED_DATA_PIN, RGB>(leds, NUM_LEDS);
  FastLED.setBrightness(100);
}

void ledEffectsShow()
{
  FastLED.show();
}

void effectSpectrumBars(const int levels[MATRIX_WIDTH])
{
  FastLED.clear();

  for (int x = 0; x < MATRIX_WIDTH; x++)
  {
    setColumn(x, levels[x], columnColors[x]);
  }
}
