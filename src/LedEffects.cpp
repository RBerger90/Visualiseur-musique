#include "LedEffects.h"
#include "Font5x7.h"
#include <string.h>

namespace
{
  const int NUM_LEDS = MATRIX_WIDTH * MATRIX_HEIGHT;
  const int LED_DATA_PIN = 18;

  CRGB leds[NUM_LEDS];

  // Base hue for PALETTE_SOLID and the gradients, adjustable via
  // ledEffectsSetHue() (POT_MODE_HUE_ROTATION mode). Starts on neon magenta.
  uint8_t baseHue = HUE_PINK;

  // Saturation applied to all palettes (rainbow included), adjustable via
  // ledEffectsSetSaturation() (POT_MODE_SATURATION mode).
  // 255 = fully saturated colors, 0 = white (no visible hue).
  uint8_t baseSaturation = 255;

  // Duration (ms) to advance one hue step (0-255) in the animated rainbow:
  // larger = slower scroll.
  const uint16_t RAINBOW_ANIM_MS_PER_HUE_STEP = 20;

  // Hue offset (out of 256) for PALETTE_GRADIENT_2's second color: 32 = 45
  // degrees, neighboring (analogous) hues rather than opposite ones.
  const int GRADIENT_2_HUE_OFFSET = 32;

  PaletteMode currentPalette = PALETTE_RAINBOW_STATIC_H;

  // Color of the LED at column x (0 to MATRIX_WIDTH-1), row y (0 to
  // MATRIX_HEIGHT-1), based on the active palette. The _H versions vary the
  // color with x (frequency), the _V versions with y (height within the
  // column, i.e. amplitude).
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
      return CHSV(rainbowHue + offset, baseSaturation, 255);
    }

    case PALETTE_SOLID:
      return CHSV(baseHue, baseSaturation, 255);

    case PALETTE_GRADIENT_1_H:
    case PALETTE_GRADIENT_1_V:
    {
      // Same hue everywhere, only the brightness varies along the axis.
      uint8_t value = map(axisValue, 0, axisMax, 60, 255);
      return CHSV(baseHue, baseSaturation, value);
    }

    case PALETTE_GRADIENT_2_H:
    case PALETTE_GRADIENT_2_V:
    {
      // Neighboring (analogous) hue rather than opposite. Kept as int to
      // keep the gradient's rotation direction stable regardless of the
      // starting hue; the final wrap happens in CHSV's cast to uint8_t.
      int secondHue = (int)baseHue - GRADIENT_2_HUE_OFFSET;
      int hue = map(axisValue, 0, axisMax, (int)baseHue, secondHue);
      return CHSV((uint8_t)hue, baseSaturation, 255);
    }

    case PALETTE_RAINBOW_STATIC_H:
    case PALETTE_RAINBOW_STATIC_V:
    default:
      return CHSV(rainbowHue, baseSaturation, 255);
    }
  }

  // The wiring snakes from one column to the next: even rows read left to
  // right, odd rows read right to left.
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

  // Position (in columns) of the start of the text; decremented to scroll
  // left. Starts off-screen on the right.
  int scrollX = MATRIX_WIDTH;
  unsigned long lastScrollStepTime = 0;
  // Time when the text reached its starting position (off-screen on the
  // right); used to mark a pause before scrolling starts.
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
        // rows[0] is the top of the character, but y=0 corresponds to the
        // physical bottom of the matrix (see setColumn): the vertical axis
        // is flipped for this mapping.
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

void ledEffectsSetHue(uint8_t hue)
{
  baseHue = hue;
}

void ledEffectsSetSaturation(uint8_t saturation)
{
  baseSaturation = saturation;
}

// Height (in LEDs) of the bar in column x for the given band value. Shared
// by all bar-based presets so they stay consistent with each other. Clamped
// here (not just where callers happen to use it): a loud transient can push
// band well past what MATRIX_HEIGHT can show, and an un-clamped height fed
// into effectSymmetricBars's "3 - d" / "4 + d" indexing wraps into negative
// or over-height rows, i.e. an out-of-bounds write into leds[] that
// corrupts memory and crashes a frame or two later in unrelated code.
int barHeight(float band)
{
  return constrain((int)(band / 10000.0), 0, MATRIX_HEIGHT);
}



void effectSpectrumBars(const float bands[MATRIX_WIDTH])
{
  FastLED.clear();

  for (int x = 0; x < MATRIX_WIDTH; x++)
    setColumn(x, barHeight(bands[x]));
}

void effectSymmetricBars(const float bands[MATRIX_WIDTH])
{
  FastLED.clear();

  for (int x = 0; x < MATRIX_WIDTH; x++)
  {
    // Fully dark at silence: half the band height grows from the exact
    // center of the matrix upward AND downward (on a height-8 matrix, a
    // height-8 bar fills the whole column and its half fills 4+4). A
    // half-height of 0 leaves the column off rather than keeping a lit
    // midline, which read as a floating artifact.
    int halfHeight = barHeight(bands[x]) / 2;

    for (int d = 0; d < halfHeight; d++)
    {
      leds[ledIndex(x, 3 - d)] = paletteColor(x, 3 - d);
      leds[ledIndex(x, 4 + d)] = paletteColor(x, 4 + d);
    }
  }
}

void effectBarsInverted(const float bands[MATRIX_WIDTH])
{
  FastLED.clear();

  for (int x = 0; x < MATRIX_WIDTH; x++)
  {
    // Same bottom-up bar as the spectrum effect, but inverted roles: the
    // bar itself is dark and the empty space above it is colored. The
    // color thus hangs from the top of the matrix down to the bar's top,
    // and grows as the bar shrinks.
    int height = constrain(barHeight(bands[x]), 0, MATRIX_HEIGHT);

    for (int y = 0; y < MATRIX_HEIGHT; y++)
      leds[ledIndex(x, y)] = (y < height) ? CRGB::Black : paletteColor(x, y);
  }
}

// Per-column peak hold for effectBarsHoppers: the white peak marker sits at
// the bar's historical maximum and falls back down slowly, lagging behind
// the (instantaneous) bar itself.
namespace
{
  int peakHeight[MATRIX_WIDTH] = {0};
  unsigned long peakLastFallMs[MATRIX_WIDTH] = {0};
}

void effectBarsHoppers(const float bands[MATRIX_WIDTH])
{
  FastLED.clear();

  const float PEAK_FALL_MS_PER_LED = 60;

  for (int x = 0; x < MATRIX_WIDTH; x++)
  {
    int height = constrain(barHeight(bands[x]), 0, MATRIX_HEIGHT);

    // The peak tracks the bar's maximum: rises instantly with the bar,
    // then falls one LED every PEAK_FALL_MS_PER_LED when the bar drops
    // below it. Falls even while the bar keeps moving, so the white dot
    // is the "recent high" of the column, slower than the bar itself.
    if (height >= peakHeight[x])
    {
      peakHeight[x] = height;
      peakLastFallMs[x] = millis();
    }
    else if (millis() - peakLastFallMs[x] >= PEAK_FALL_MS_PER_LED)
    {
      peakHeight[x]--;
      peakLastFallMs[x] = millis();
    }
    peakHeight[x] = constrain(peakHeight[x], 0, MATRIX_HEIGHT);

    setColumn(x, height);

    // White peak marker at the top of the peak (the bar's recent max).
    // Drawn AFTER setColumn, overwriting the bar's top LED when the bar
    // has caught up with its peak. On a rising bar the peak is the bar
    // top itself, so the dot reads as the bar's leading edge.
    int peakY = peakHeight[x] - 1;
    if (peakY >= 0)
      leds[ledIndex(x, peakY)] = CRGB::White;
  }
}

void effectScrollingText(const char *text, CRGB color, uint16_t stepIntervalMs)
{
  int textLength = strlen(text);
  int textPixelWidth = textLength * CHAR_STEP;

  FastLED.clear();

  // The text fits in the grid: it stays displayed, centered, without scrolling.
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

void effectParamHud(int modeIndex, int barLevel, CRGB barColor, bool barFilled)
{
  modeIndex = constrain(modeIndex, 0, MATRIX_HEIGHT - 1);

  for (int y = 0; y < MATRIX_HEIGHT; y++)
  {
    bool isModeRow = y == MATRIX_HEIGHT - 1 - modeIndex;
    leds[ledIndex(0, y)] = isModeRow ? CRGB::MediumVioletRed : CRGB::Black;
  }

  for (int y = 0; y < MATRIX_HEIGHT; y++)
  {
    bool lit;
    if (barLevel < 0)
      lit = false;
    else if (barFilled)
      lit = y <= constrain(barLevel, 0, MATRIX_HEIGHT - 1);
    else
      lit = y == constrain(barLevel, 0, MATRIX_HEIGHT - 1);

    leds[ledIndex(1, y)] = lit ? barColor : CRGB::Black;
  }

  for (int y = 0; y < MATRIX_HEIGHT; y++)
    leds[ledIndex(2, y)] = CRGB::Black;
}