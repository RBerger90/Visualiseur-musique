#include "LedEffects.h"
#include "Font5x7.h"
#include <string.h>

namespace
{
  const int NUM_LEDS = MATRIX_WIDTH * MATRIX_HEIGHT;
  const int LED_DATA_PIN = 18;

  CRGB leds[NUM_LEDS];

  // Teintes de depart pour les palettes unie/degradees (ambiance "neon").
  // A terme, ces teintes pourront devenir ajustables (ex: complementaire
  // auto calculee depuis une seule teinte de base).
  const uint8_t SOLID_HUE = HUE_PINK;
  const uint8_t GRADIENT_HUE_A = HUE_PINK;
  const uint8_t GRADIENT_HUE_B = HUE_GREEN;

  // Duree (ms) pour avancer d'un pas de teinte (0-255) en arc-en-ciel
  // anime : plus grand = defilement plus lent.
  const uint16_t RAINBOW_ANIM_MS_PER_HUE_STEP = 20;

  PaletteMode currentPalette = PALETTE_RAINBOW_STATIC_H;

  // Couleur de la LED en colonne x (0 a MATRIX_WIDTH-1), ligne y (0 a
  // MATRIX_HEIGHT-1), selon la palette active. Les versions _H font varier
  // la couleur selon x (la frequence), les versions _V selon y (la hauteur
  // dans la colonne, donc l'amplitude).
  CRGB paletteColor(int x, int y)
  {
    bool vertical = currentPalette == PALETTE_RAINBOW_STATIC_V ||
                    currentPalette == PALETTE_RAINBOW_ANIMATED_V ||
                    currentPalette == PALETTE_GRADIENT_1_V ||
                    currentPalette == PALETTE_GRADIENT_2_V;

    int axisValue = vertical ? y : x;
    int axisMax = (vertical ? MATRIX_HEIGHT : MATRIX_WIDTH) - 1;
    uint8_t rainbowHue = (uint8_t)(axisValue * 256 / (axisMax + 1));

    switch (currentPalette)
    {
    case PALETTE_RAINBOW_ANIMATED_H:
    case PALETTE_RAINBOW_ANIMATED_V:
    {
      uint8_t offset = (uint8_t)(millis() / RAINBOW_ANIM_MS_PER_HUE_STEP);
      return CHSV(rainbowHue + offset, 255, 255);
    }

    case PALETTE_SOLID:
      return CHSV(SOLID_HUE, 255, 255);

    case PALETTE_GRADIENT_1_H:
    case PALETTE_GRADIENT_1_V:
    {
      // Meme teinte partout, seule la luminosite varie le long de l'axe.
      uint8_t value = map(axisValue, 0, axisMax, 60, 255);
      return CHSV(GRADIENT_HUE_A, 255, value);
    }

    case PALETTE_GRADIENT_2_H:
    case PALETTE_GRADIENT_2_V:
    {
      uint8_t hue = map(axisValue, 0, axisMax, GRADIENT_HUE_A, GRADIENT_HUE_B);
      return CHSV(hue, 255, 255);
    }

    case PALETTE_RAINBOW_STATIC_H:
    case PALETTE_RAINBOW_STATIC_V:
    default:
      return CHSV(rainbowHue, 255, 255);
    }
  }

  // Le cablage serpente d'une colonne a l'autre : les lignes paires se lisent
  // de gauche a droite, les lignes impaires de droite a gauche.
  int ledIndex(int x, int y)
  {
    if (y % 2 == 0)
      return y * MATRIX_WIDTH + x;
    return y * MATRIX_WIDTH + (MATRIX_WIDTH - 1 - x);
  }

  void setColumn(int x, int height)
  {
    height = constrain(height, 0, MATRIX_HEIGHT);

    for (int y = 0; y < MATRIX_HEIGHT; y++)
    {
      leds[ledIndex(x, y)] = (y < height) ? paletteColor(x, y) : CRGB::Black;
    }
  }

  const int CHAR_SPACING = 1;
  const int CHAR_STEP = FONT_WIDTH + CHAR_SPACING;
  const unsigned long SCROLL_START_HOLD_MS = 1000;

  // Position (en colonnes) du debut du texte ; decremente pour defiler vers
  // la gauche. Demarre hors ecran a droite.
  int scrollX = MATRIX_WIDTH;
  unsigned long lastScrollStepTime = 0;
  // Instant ou le texte est arrive a sa position de depart (hors ecran a
  // droite) ; sert a marquer une pause avant de commencer a defiler.
  unsigned long scrollCycleStartTime = 0;
  bool scrollCycleStarted = false;

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
  ledEffectsSetBrightness(30);
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

void ledEffectsSetPalette(PaletteMode mode)
{
  currentPalette = mode;
}

void effectSpectrumBars(const float bands[MATRIX_WIDTH])
{
  FastLED.clear();

  for (int x = 0; x < MATRIX_WIDTH; x++)
  {
    int height = (int)(bands[x] / 10000.0);
    setColumn(x, height);
  }
}

void effectScrollingText(const char *text, CRGB color, uint16_t stepIntervalMs)
{
  int textLength = strlen(text);
  int textPixelWidth = textLength * CHAR_STEP;

  FastLED.clear();

  // Le texte tient dans la grille : il reste affiche, centre, sans defiler.
  if (textPixelWidth <= MATRIX_WIDTH)
  {
    scrollX = MATRIX_WIDTH;
    scrollCycleStarted = false;

    int startX = (MATRIX_WIDTH - textPixelWidth) / 2;
    for (int i = 0; i < textLength; i++)
      drawGlyphAt(startX + i * CHAR_STEP, getGlyphRows(text[i]), color);
    return;
  }

  if (!scrollCycleStarted)
  {
    scrollCycleStartTime = millis();
    scrollCycleStarted = true;
  }

  bool holdingAtStart = millis() - scrollCycleStartTime < SCROLL_START_HOLD_MS;

  if (!holdingAtStart && millis() - lastScrollStepTime >= stepIntervalMs)
  {
    lastScrollStepTime = millis();
    scrollX--;
    if (scrollX < -textPixelWidth)
    {
      scrollX = MATRIX_WIDTH;
      scrollCycleStarted = false;
    }
  }

  for (int i = 0; i < textLength; i++)
  {
    int charX = scrollX + i * CHAR_STEP;
    if (charX + FONT_WIDTH < 0 || charX >= MATRIX_WIDTH)
      continue;

    drawGlyphAt(charX, getGlyphRows(text[i]), color);
  }
}

void effectFillSnake(const int ledCount, const CRGB color)
{
  FastLED.clear();

  for (int i = 0; i < ledCount && i < NUM_LEDS; i++)
  {
    leds[i] = color;
  }
}