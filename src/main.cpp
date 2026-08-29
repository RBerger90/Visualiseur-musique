#include <Arduino.h>
#include <driver/i2s.h>
#include <arduinoFFT.h>
#include "Config.h"
#include "LedEffects.h"
#include "Encoder.h"
#include "Button.h"

#define I2S_WS 15
#define I2S_SD 13
#define I2S_SCK 2
#define I2S_PORT I2S_NUM_0
#define BUFFER_LEN 256

#define ENCODER_CLK_PIN 32
#define ENCODER_DT_PIN 33
#define BUTTON_PIN 27

// Mic sensitivity levels selectable via the encoder, one detent = one
// level. Tighter around 1.0x (the default value), wider apart toward the
// extremes.
const float MIC_SENSITIVITY_LEVELS[] = {0.5, 0.75, 1.0, 1.25, 1.5, 2.0, 2.5, 3.0};
const int MIC_SENSITIVITY_LEVEL_COUNT =
    sizeof(MIC_SENSITIVITY_LEVELS) / sizeof(MIC_SENSITIVITY_LEVELS[0]);

// What the encoder currently modifies; its button (a click on the shaft)
// advances to the next mode. The order declared here is the click cycle
// order.
enum PotMode
{
  POT_MODE_MIC_SENSITIVITY,
  POT_MODE_ANIM_SPEED,
  POT_MODE_COLOR_PALETTE,
  POT_MODE_HUE_ROTATION,
  POT_MODE_SATURATION,
  POT_MODE_COUNT
};

PotMode potMode = POT_MODE_MIC_SENSITIVITY;

// Current index into MIC_SENSITIVITY_LEVELS (2 = 1.0x, the default value).
int micSensitivityLevel = 2;

// Multiplies the frequency band amplitude in computeBands(): higher = quiet
// sounds move the matrix more.
float micSensitivity = MIC_SENSITIVITY_LEVELS[micSensitivityLevel];

// Delay (ms) added to every frame on top of the normal processing time, to
// slow down the overall display rate and make it less strobe-like.
// Descending order (index 0 = slowest) so animSpeedLevel follows the same
// rotation convention as the other modes (a detent in the "increase"
// direction also fills up the HUD bar, no separate inversion needed).
const uint16_t FRAME_DELAY_LEVELS_MS[] = {100, 75, 50, 35, 20, 10, 5, 0};
const int FRAME_DELAY_LEVEL_COUNT =
    sizeof(FRAME_DELAY_LEVELS_MS) / sizeof(FRAME_DELAY_LEVELS_MS[0]);

// Current index into FRAME_DELAY_LEVELS_MS (last index = 0ms delay, the
// behavior before this setting existed).
int animSpeedLevel = FRAME_DELAY_LEVEL_COUNT - 1;

// Palette currently used by effectSpectrumBars().
PaletteMode colorPalette = PALETTE_RAINBOW_STATIC_H;

// Step applied to baseHue (0-255) per encoder detent in rotation mode.
const uint8_t HUE_ROTATION_STEP = 4;

// Current base hue (source color for PALETTE_SOLID and the gradients).
uint8_t baseHue = HUE_PINK;

// Step applied to saturation (0-255) per encoder detent. Unlike baseHue,
// this doesn't loop: going past an extremity wouldn't make sense here (255
// doesn't "become" 0 again once exceeded).
const int SATURATION_STEP = 8;

// Current saturation of the palette colors (255 = fully saturated).
uint8_t saturation = 255;

// Settings indicator (effectParamHud): stays displayed for this long after
// the last change (click or rotation), then disappears.
const unsigned long PARAM_HUD_DURATION_MS = 3000;

// Stays false until a change has ever happened, so the indicator doesn't
// show at startup.
bool paramHudTriggered = false;
unsigned long paramHudShownAt = 0;

const uint16_t SAMPLES = 512;
const uint32_t SAMPLE_RATE = 16000;

double vReal[SAMPLES];
double vImag[SAMPLES];
float bands[MATRIX_WIDTH];

ArduinoFFT<double> FFT(vReal, vImag, SAMPLES, SAMPLE_RATE);

int32_t rawSamples[SAMPLES];

int32_t convertSample(int32_t x)
{
  x = x >> 8;
  if (x & 0x00800000)
    x |= 0xFF000000;
  return x;
}

int32_t samples[BUFFER_LEN];

void setupI2S()
{
  i2s_config_t i2s_config = {
      .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
      .sample_rate = 44100,
      .bits_per_sample = i2s_bits_per_sample_t(16),
      .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
      .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_STAND_I2S),
      .intr_alloc_flags = 0, // default interrupt priority
      .dma_buf_count = 8,
      .dma_buf_len = BUFFER_LEN,
      .use_apll = false};

  i2s_pin_config_t pin_config = {
      .bck_io_num = I2S_SCK,
      .ws_io_num = I2S_WS,
      .data_out_num = I2S_PIN_NO_CHANGE,
      .data_in_num = I2S_SD};

  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_PORT, &pin_config);
  i2s_zero_dma_buffer(I2S_PORT);
}

void captureSamples()
{
  size_t bytesRead = 0;
  i2s_read(I2S_PORT, rawSamples, sizeof(rawSamples), &bytesRead, portMAX_DELAY);

  for (int i = 0; i < SAMPLES; i++)
  {
    vReal[i] = (double)convertSample(rawSamples[i]);
    vImag[i] = 0.0;
  }
}

void computeBands()
{
  const float bandGain[MATRIX_WIDTH] = {
      0.20, 0.25, 0.35, 0.50, 0.65, 0.80,
      0.90, 1.00, 1.00, 1.00, 1.00, 1.00,
      1.05, 1.05, 1.10, 1.10, 1.15, 1.15,
      1.20, 1.20, 1.25, 1.25, 1.30, 1.30};

  const float noiseFloor[MATRIX_WIDTH] = {
      2500, 2200, 1800, 1500, 1200, 1000,
      900, 800, 750, 700, 650, 650,
      600, 600, 550, 550, 500, 500,
      500, 500, 450, 450, 450, 450};

  for (int i = 0; i < MATRIX_WIDTH; i++)
    bands[i] = 0;
  int counts[MATRIX_WIDTH] = {0};

  FFT.windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.compute(FFT_FORWARD);
  FFT.complexToMagnitude();

  for (int bin = 6; bin < SAMPLES / 2; bin++)
  {
    int band = map(bin, 6, (SAMPLES / 2) - 1, 0, MATRIX_WIDTH - 1);
    bands[band] += vReal[bin];
    counts[band]++;
  }

  for (int i = 0; i < MATRIX_WIDTH; i++)
  {
    if (counts[i] > 0)
      bands[i] /= counts[i];

    bands[i] -= noiseFloor[i];
    if (bands[i] < 0)
      bands[i] = 0;

    bands[i] *= bandGain[i] * micSensitivity;
  }
}

void removeDC()
{
  double mean = 0;

  for (int i = 0; i < SAMPLES; i++)
  {
    mean += vReal[i];
  }
  mean /= SAMPLES;

  for (int i = 0; i < SAMPLES; i++)
  {
    vReal[i] -= mean;
  }
}

void setup()
{
  Serial.begin(115200);
  delay(500);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  encoderSetup(ENCODER_CLK_PIN, ENCODER_DT_PIN);
  setupI2S();
  ledEffectsSetup();
}

void loop()
{
  if (buttonPressed(BUTTON_PIN))
  {
    potMode = (PotMode)((potMode + 1) % POT_MODE_COUNT);
    Serial.print("pot mode = ");
    Serial.println(potMode);

    paramHudTriggered = true;
    paramHudShownAt = millis();
  }

  // Drain the encoder every frame even outside mic sensitivity mode, so a
  // movement made in another mode doesn't get applied all at once when
  // coming back to this one.
  int encoderDelta = encoderRead();

  if (encoderDelta != 0)
  {
    paramHudTriggered = true;
    paramHudShownAt = millis();
  }

  switch (potMode)
  {
  case POT_MODE_MIC_SENSITIVITY:
    // One detent = one level; clamped at the extremities (no wraparound, a
    // sensitivity doesn't "restart" at 0.5x once 3.0x is exceeded).
    micSensitivityLevel = constrain(
        micSensitivityLevel + encoderDelta, 0, MIC_SENSITIVITY_LEVEL_COUNT - 1);
    micSensitivity = MIC_SENSITIVITY_LEVELS[micSensitivityLevel];
    break;
  case POT_MODE_ANIM_SPEED:
    // One detent = one level; clamped like micSensitivity (no wraparound, a
    // speed doesn't "become" the slowest again once the fastest is
    // exceeded). The delay is applied at the end of loop(), not here.
    animSpeedLevel = constrain(
        animSpeedLevel + encoderDelta, 0, FRAME_DELAY_LEVEL_COUNT - 1);
    break;
  case POT_MODE_COLOR_PALETTE:
    // One detent = the next/previous palette, with circular wraparound
    // (unlike micSensitivity, a palette has no bounds).
    if (encoderDelta != 0)
    {
      int nextPalette = ((int)colorPalette + encoderDelta) % PALETTE_MODE_COUNT;
      if (nextPalette < 0)
        nextPalette += PALETTE_MODE_COUNT;

      colorPalette = (PaletteMode)nextPalette;
      ledEffectsSetPalette(colorPalette);
    }
    break;
  case POT_MODE_HUE_ROTATION:
    // baseHue is a uint8_t: the addition wraps around naturally on 0-255,
    // no need to clamp it like micSensitivityLevel.
    baseHue += encoderDelta * HUE_ROTATION_STEP;
    ledEffectsSetHue(baseHue);
    break;
  case POT_MODE_SATURATION:
    saturation = constrain((int)saturation + encoderDelta * SATURATION_STEP, 0, 255);
    ledEffectsSetSaturation(saturation);
    break;
  default:
    // Modes not wired to the encoder yet.
    break;
  }

  captureSamples();
  removeDC();
  computeBands();

  effectSpectrumBars(bands);

  bool paramHudVisible = paramHudTriggered &&
                          millis() - paramHudShownAt < PARAM_HUD_DURATION_MS;
  if (paramHudVisible)
  {
    // Palette doesn't have a displayable value yet: -1 leaves the value
    // column off for that mode.
    int barLevel = -1;
    if (potMode == POT_MODE_MIC_SENSITIVITY)
      barLevel = micSensitivityLevel;
    else if (potMode == POT_MODE_ANIM_SPEED)
      barLevel = animSpeedLevel;
    else if (potMode == POT_MODE_HUE_ROTATION)
      barLevel = map(baseHue, 0, 255, 0, MATRIX_HEIGHT - 1);
    else if (potMode == POT_MODE_SATURATION)
      barLevel = map(saturation, 0, 255, 0, MATRIX_HEIGHT - 1);

    effectParamHud((int)potMode, barLevel);
  }

  ledEffectsShow();

  uint16_t frameDelayMs = FRAME_DELAY_LEVELS_MS[animSpeedLevel];
  if (frameDelayMs > 0)
    delay(frameDelayMs);
}