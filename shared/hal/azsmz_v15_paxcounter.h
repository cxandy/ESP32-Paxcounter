// clang-format off
// upload_speed 921600
// board esp32dev

#ifndef _AZSMZ_V15_PAXCOUNTER_H
#define _AZSMZ_V15_PAXCOUNTER_H

#include <stdint.h>

// AZSMZ ESP32 Matrix Controller v1.5 (github.com/cxandy/AZSMZ-ESPMatrixPanel)
// running Paxcounter.
//
// The on-board ESP32-WROOM-32 drives a 64x32 HUB75 RGB panel through
// SmartMatrix4-ESP32 in I2S "LCD" mode with DMA, so the refresh runs in
// hardware and costs almost no CPU.
//
// Panel pins are selected by the driver itself via GPIOPINOUT, and are:
//
//   R1=17  G1=2   B1=16  R2=4   G2=15  B2=12
//   A=26   B=13   C=14   D=27   E=22
//   LAT=33 OE=32  CLK=25
//
// The SD card sits on the otherwise unused VSPI pins. No overlap with the
// panel, and no peripheral conflict either: the panel uses I2S, the SD card
// uses SPI/VSPI.
//
//   SD CLK=18  MISO=19  MOSI=23  CS=5

//#define HAS_LORA 1 // no LoRa radio on this board
//#define HAS_SPI 1  // no SPI slave uplink
//#define HAS_MQTT 1 // no uplink

// local paxcount table on the SD card
#define HAS_SDCARD  1      // SD-card-reader/writer, SPI host mode
#define SDCARD_CS    (5)
#define SDCARD_MOSI  (23)
#define SDCARD_MISO  (19)
#define SDCARD_SCLK  (18)

// 64x32 RGB LED matrix driven by SmartMatrix / I2S DMA
#define HAS_SMARTMATRIX           1       // SmartMatrix4-ESP32 display output
#define LED_MATRIX_WIDTH          (64)    // Width (cols) in pixels of the panel
#define LED_MATRIX_HEIGHT         (32)    // Height (rows) in pixels of the panel
#define MATRIX_DISPLAY_BRIGHTNESS 30      // panel brightness 0..255
#define MATRIX_DISPLAY_TREND_LEN  32      // number of bars in the trend graph

//#define BOARD_HAS_PSRAM // WROOM-32 has no PSRAM
#define DISABLE_BROWNOUT 1 // comment out if you want to keep brownout feature

// GPIO2 carries the panel's G1 data line, so the on board LED cannot be used
#define HAS_LED NOT_A_PIN
//#define HAS_BUTTON (0)  // BOOT button on GPIO0

// Nothing on this board uses I2C, and the default SDA/SCL pins (GPIO21/GPIO22)
// would fight the panel: GPIO22 is the E row-address line. Keep the bus closed.
#define NO_I2C_BUS

// No battery voltage divider on this board, so skip the ADC probe:
//#define BAT_MEASURE_ADC ADC1_GPIO35_CHANNEL
//#define BAT_VOLTAGE_DIVIDER 2

#endif