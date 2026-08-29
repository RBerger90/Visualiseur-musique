#pragma once

#include <FastLED.h>
#include "Config.h"

// Call once in setup(): initializes the LED strip/matrix.
void ledEffectsSetup();

// Call every frame, after filling the LED buffer with an effect.
void ledEffectsShow();

// Sets the overall strip brightness (0 = off, 255 = max brightness).
void ledEffectsSetBrightness(int brightness);

// Palettes available for effectSpectrumBars(). The order is the one the
// encoder cycles through (POT_MODE_COLOR_PALETTE mode).
// Each shape (rainbow, gradients) exists in a horizontal version (varies
// with the column, i.e. frequency) and a vertical one (varies with the
// height within the column, i.e. amplitude). PALETTE_SOLID only has one
// version: a solid color doesn't change with the chosen orientation.
enum PaletteMode
{
  PALETTE_RAINBOW_STATIC_H,
  PALETTE_RAINBOW_STATIC_V,
  PALETTE_RAINBOW_ANIMATED_H,
  PALETTE_RAINBOW_ANIMATED_V,
  PALETTE_SOLID,
  PALETTE_GRADIENT_1_H,
  PALETTE_GRADIENT_1_V,
  PALETTE_GRADIENT_2_H,
  PALETTE_GRADIENT_2_V,
  PALETTE_MODE_COUNT
};

// Changes the palette used by effectSpectrumBars().
void ledEffectsSetPalette(PaletteMode mode);

// Base hue (0-255) used by PALETTE_SOLID and the two gradients:
// PALETTE_SOLID and PALETTE_GRADIENT_1 use it as-is, PALETTE_GRADIENT_2
// gradients from this hue to a neighboring hue (analogous, computed
// automatically from this single base hue).
void ledEffectsSetHue(uint8_t hue);

// Saturation (0-255) applied to all palettes, rainbow included.
// 255 = fully saturated colors, 0 = white.
void ledEffectsSetSaturation(uint8_t saturation);

// --- Effects ---
// Each effect reads the audio data (one value per column, computed from the
// microphone) and updates the LED buffer accordingly. ledEffectsShow() must
// then be called to send the result to the strip.
// This is where the next effects should be added (one per function).

// Spectrum bars: each column lights up to a height computed from bands[x]
// (FFT magnitude of the column). The color of each LED depends on the
// active palette (see PaletteMode), by column and/or row.
void effectSpectrumBars(const float bands[MATRIX_WIDTH]);

// Scrolls `text` right to left, one column less every stepIntervalMs. The
// scroll state is kept internally (only one scroll active at a time): call
// this function every frame to advance the animation, the text restarts
// from the right once it's scrolled off the left.
void effectScrollingText(const char *text, CRGB color, uint16_t stepIntervalMs = 80);

void effectFillSnake(const int ledCount, const CRGB color);

// Overlays a settings indicator on the first 3 columns (call after the main
// effect, to replace those columns rather than overlaying them):
// - column 0: which parameter is being adjusted. A single lit LED, at row
//   modeIndex counting from the top (0 = topmost).
// - column 1: that parameter's value, as a bar filled from the bottom.
//   barLevel ranges from 0 to MATRIX_HEIGHT-1; barLevel < 0 means this mode
//   doesn't have a displayable value yet, the column stays off.
// - column 2: always off, separates the indicator from the rest of the
//   visual.
void effectParamHud(int modeIndex, int barLevel);
