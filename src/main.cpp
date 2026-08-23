#include <Arduino.h>
#include <driver/i2s.h>
#include <arduinoFFT.h>
#include "Config.h"
#include "LedEffects.h"
#include "Potentiometer.h"
#include "Button.h"

#define I2S_WS 15
#define I2S_SD 13
#define I2S_SCK 2
#define I2S_PORT I2S_NUM_0
#define BUFFER_LEN 256

#define POT2_PIN 34
#define BUTTON_PIN 27

// Ce que la molette 2 modifie actuellement ; le bouton (place sous la
// molette, simule son clic) fait avancer ce mode.
enum PotMode
{
  POT_MODE_MIC_SENSITIVITY,
  POT_MODE_COLOR_PALETTE,
  POT_MODE_SATURATION,
  POT_MODE_ANIM_SPEED,
  POT_MODE_COUNT
};

PotMode potMode = POT_MODE_MIC_SENSITIVITY;

// Multiplie l'amplitude des bandes de frequence dans computeBands() : plus
// haut = les sons faibles font davantage bouger la matrice.
float micSensitivity = 1.0;

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
  }

  switch (potMode)
  {
  case POT_MODE_MIC_SENSITIVITY:
    // Plage 0.5x a 3.0x, potRead renvoie un entier donc on travaille en
    // centiemes puis on redivise.
    micSensitivity = potRead(POT2_PIN, 50, 300) / 100.0;
    break;
  default:
    // Modes pas encore branches sur la molette.
    break;
  }

  captureSamples();
  removeDC();
  computeBands();

  effectSpectrumBars(bands);
  ledEffectsShow();
}