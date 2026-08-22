// #include <Arduino.h>
// #include <driver/i2s.h>
// #include <FastLED.h>

// #define I2S_WS 15
// #define I2S_SD 13
// #define I2S_SCK 2
// #define I2S_PORT I2S_NUM_0
// #define bufferLen 64

// #define NUM_LEDS 300
// #define DATA_PIN 18
// CRGB leds[NUM_LEDS];

// int16_t sBuffer[bufferLen];

// void i2s_install()
// {
//   const i2s_config_t i2s_config = {
//       .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
//       .sample_rate = 44100,
//       .bits_per_sample = i2s_bits_per_sample_t(16),
//       .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
//       .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_STAND_I2S),
//       .intr_alloc_flags = 0, // default interrupt priority
//       .dma_buf_count = 8,
//       .dma_buf_len = bufferLen,
//       .use_apll = false};

//   i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
// }

// void i2s_setpin()
// {
//   const i2s_pin_config_t pin_config = {
//       .bck_io_num = I2S_SCK,
//       .ws_io_num = I2S_WS,
//       .data_out_num = -1,
//       .data_in_num = I2S_SD};

//   i2s_set_pin(I2S_PORT, &pin_config);
// }

// void setup()
// {
//   Serial.begin(115200);
//   Serial.println("Setup I2S ...");

//   delay(1000);
//   i2s_install();
//   i2s_setpin();
//   i2s_start(I2S_PORT);
//   delay(500);

//   FastLED.addLeds<WS2811, DATA_PIN, RGB>(leds, NUM_LEDS);
//   FastLED.setBrightness(80); // Lumineux faible
// }

// void loop()
// {

//   size_t bytesIn = 0;
//   esp_err_t result = i2s_read(I2S_PORT, &sBuffer, bufferLen, &bytesIn, portMAX_DELAY);
//   if (result == ESP_OK)
//   {
//     int samples_read = bytesIn / 8;
//     if (samples_read > 0)
//     {
//       float mean = 0;
//       for (int i = 0; i < samples_read; ++i)
//       {
//         mean += (sBuffer[i]);
//       }
//       mean /= samples_read;
//       Serial.print(">stp:");
//       Serial.println(mean); // Writes \r\n
//       delay(50);
//     }
//   }

//   // Blanc fixe 1s
//   // fill_solid(leds, NUM_LEDS, CRGB::GreenYellow);
//   // FastLED.show();
//   // delay(1000);

//   // // Blanc fixe 1s
//   // fill_solid(leds, NUM_LEDS, CRGB::DeepPink1);
//   // FastLED.show();
//   // delay(1000);
// }
