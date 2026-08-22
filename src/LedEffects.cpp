#include "LedEffects.h"
#include "Font5x7.h"
#include <string.h>

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

  const int CHAR_SPACING = 1;
  const int CHAR_STEP = FONT_WIDTH + CHAR_SPACING;

  // Position (en colonnes) du debut du texte ; decremente pour defiler vers
  // la gauche. Demarre hors ecran a droite.
  int scrollX = MATRIX_WIDTH;
  unsigned long lastScrollStepTime = 0;

  void drawGlyphAt(int startX, const uint8_t *rows, CRGB color)
  {
    for (int y = 0; y < FONT_HEIGHT && y < MATRIX_HEIGHT; y++)
    {
      uint8_t rowBits = rows[y];

      for (int col = 0; col < FONT_WIDTH; col++)
      {
        int x = startX + col;
        if (x < 0 || x >= MATRIX_WIDTH)
          continue;

        bool pixelOn = rowBits & (1 << (FONT_WIDTH - 1 - col));
        // rows[0] est le haut du caractere, mais y=0 correspond au bas physique de la
        // matrice (cf. setColumn) : on inverse l'axe vertical pour ce mapping-la.
        if (pixelOn)
          leds[ledIndex(x, MATRIX_HEIGHT - 1 - y)] = color;
      }
    }
  }
}

void ledEffectsSetup()
{
  FastLED.addLeds<WS2811, LED_DATA_PIN, RGB>(leds, NUM_LEDS);
  ledEffectsSetBrightness(10);
}

void ledEffectsShow()
{
  FastLED.show();
}

void ledEffectsSetBrightness(int brightness)
{
  brightness = constrain(brightness, 0, 255);
  FastLED.setBrightness(brightness);
}

void effectSpectrumBars(const int levels[MATRIX_WIDTH])
{
  FastLED.clear();

  for (int x = 0; x < MATRIX_WIDTH; x++)
  {
    setColumn(x, levels[x], columnColors[x]);
  }
}

void effectScrollingText(const char *text, CRGB color, uint16_t stepIntervalMs)
{
  int textLength = strlen(text);
  int textPixelWidth = textLength * CHAR_STEP;

  if (millis() - lastScrollStepTime >= stepIntervalMs)
  {
    lastScrollStepTime = millis();
    scrollX--;
    if (scrollX < -textPixelWidth)
      scrollX = MATRIX_WIDTH;
  }

  FastLED.clear();

  for (int i = 0; i < textLength; i++)
  {
    int charX = scrollX + i * CHAR_STEP;
    if (charX + FONT_WIDTH < 0 || charX >= MATRIX_WIDTH)
      continue;

    drawGlyphAt(charX, getGlyphRows(text[i]), color);
  }
}
