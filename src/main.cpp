#include <Arduino.h>
#include <driver/i2s.h>
#include <arduinoFFT.h>
#include <FastLED.h>

#define I2S_WS 15
#define I2S_SD 13
#define I2S_SCK 2
#define I2S_PORT I2S_NUM_0
#define BUFFER_LEN 256

#define NUM_LEDS 192
#define DATA_PIN 18
CRGB leds[NUM_LEDS];

const int WIDTH = 24;
const int HEIGHT = 8;

const uint16_t SAMPLES = 512;
const uint32_t SAMPLE_RATE = 16000;

double vReal[SAMPLES];
double vImag[SAMPLES];
float bands[WIDTH];

int levels[WIDTH] = {
    2,
    4,
    2,
    3,
    0,
    5,
    5,
    5,
    6,
    3,
    4,
    4,
    5,
    6,
    7,
    2,
    4,
    6,
    8,
    0,
    1,
    2,
    3,
    4,
};

CRGB colors[WIDTH] = {
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
  const float bandGain[WIDTH] = {
      0.20, 0.25, 0.35, 0.50, 0.65, 0.80,
      0.90, 1.00, 1.00, 1.00, 1.00, 1.00,
      1.05, 1.05, 1.10, 1.10, 1.15, 1.15,
      1.20, 1.20, 1.25, 1.25, 1.30, 1.30};

  const float noiseFloor[WIDTH] = {
      2500, 2200, 1800, 1500, 1200, 1000,
      900, 800, 750, 700, 650, 650,
      600, 600, 550, 550, 500, 500,
      500, 500, 450, 450, 450, 450};

  for (int i = 0; i < WIDTH; i++)
    bands[i] = 0;
  int counts[WIDTH] = {0};

  FFT.windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.compute(FFT_FORWARD);
  FFT.complexToMagnitude();

  for (int bin = 6; bin < SAMPLES / 2; bin++)
  {
    int band = map(bin, 6, (SAMPLES / 2) - 1, 0, WIDTH - 1);
    bands[band] += vReal[bin];
    counts[band]++;
  }

  for (int i = 0; i < WIDTH; i++)
  {
    if (counts[i] > 0)
      bands[i] /= counts[i];

    bands[i] -= noiseFloor[i];
    if (bands[i] < 0)
      bands[i] = 0;

    bands[i] *= bandGain[i];
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

int XY(int x, int y)
{
  if (y % 2 == 0)
  {
    return y * WIDTH + x;
  }
  else
  {
    return y * WIDTH + (WIDTH - 1 - x);
  }
}

void setColumn(int x, int h, CRGB color)
{
  h = constrain(h, 0, HEIGHT);

  for (int y = 0; y < HEIGHT; y++)
  {
    if (y < h)
    {
      leds[XY(x, y)] = color;
    }
    else
    {
      leds[XY(x, y)] = CRGB::Black;
    }
  }
}

void printBands()
{
  // fakeClearScreen();
  Serial.println("---- SPECTRUM ----");

  for (int i = 0; i < WIDTH; i++)
  {
    int len = (int)(bands[i] / 10000.0);
    if (len < 0)
      len = 0;
    if (len > HEIGHT)
      len = HEIGHT;

    if (i < 10)
      Serial.print("0");
    Serial.print(i);
    Serial.print(" len: ");
    Serial.print(len);
    levels[i] = len;

    // for (int j = 0; j < len; j++)
    // {
    //   Serial.print("|");
    // }
    Serial.println();
  }
}

void switchLeds()
{
}

void setup()
{
  Serial.begin(115200);
  delay(500);
  setupI2S();

  // i2s_config_t i2s_config = {
  //     .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
  //     .sample_rate = 44100,
  //     .bits_per_sample = i2s_bits_per_sample_t(16),
  //     .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
  //     .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_STAND_I2S),
  //     .intr_alloc_flags = 0, // default interrupt priority
  //     .dma_buf_count = 8,
  //     .dma_buf_len = BUFFER_LEN,
  //     .use_apll = false};

  // i2s_pin_config_t pin_config = {
  //     .bck_io_num = I2S_SCK,
  //     .ws_io_num = I2S_WS,
  //     .data_out_num = I2S_PIN_NO_CHANGE,
  //     .data_in_num = I2S_SD};

  // i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  // i2s_set_pin(I2S_PORT, &pin_config);
  // i2s_zero_dma_buffer(I2S_PORT);

  FastLED.addLeds<WS2811, DATA_PIN, RGB>(leds, NUM_LEDS);
  FastLED.setBrightness(100); // Lumineux faible
}

void loop()
{
  captureSamples();
  removeDC();
  computeBands();
  printBands();

  FastLED.clear();

  for (int x = 0; x < WIDTH; x++)
  {
    setColumn(x, levels[x], colors[x]);
  }

  FastLED.show();

  Serial.print("bands = ");
  for (int x = 0; x < WIDTH; x++)
  {
    Serial.print(bands[x]);
    Serial.print(" ");
  }
}