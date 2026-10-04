// clang-format off
// upload_speed 115200
// board esp32dev

#ifndef _ESP32DEV_SD_H
#define _ESP32DEV_SD_H

#include <stdint.h>

// Hardware related definitions for a classic ESP32 devkit without LoRa radio,
// with a SPI SD-card reader wired to the default VSPI pins:
//
//   SD CLK  -> IO18
//   SD MISO -> IO19
//   SD MOSI -> IO23
//   SD CS   -> IO5
//
// No uplink channel is configured (no LoRa, no SPI slave, no MQTT), so the
// paxcount data is only written to the SD card as CSV.

// enable only if device shall not send data via LoRa or has no LoRa
//#define HAS_LORA 1
//#define HAS_SPI 1  // SPI slave uplink

// enable only if you want to store a local paxcount table on the device
#define HAS_SDCARD  1      // SD-card-reader/writer, SPI host mode
// Pins for SD-card
#define SDCARD_CS    (5)
#define SDCARD_MOSI  (23)
#define SDCARD_MISO  (19)
#define SDCARD_SCLK  (18)

// enable only if device has these sensors, otherwise comment these lines

// BME280 sensor on I2C bus
//#define HAS_BME 1 // Enable BME sensors in general
//#define HAS_BME280 GPIO_NUM_21, GPIO_NUM_22 // SDA, SCL
//#define BME280_ADDR 0x76 // change to 0x77 depending on your wiring

// SDS011 dust sensor settings
//#define HAS_SDS011 1 // use SDS011
// used pins on the ESP-side:
//#define SDS_TX 19     // connect to RX on the SDS011
//#define SDS_RX 23     // connect to TX on the SDS011

// up to three user defined sensors (if connected)
//#define HAS_SENSOR_1 1 // comment out if device has user defined sensor #1
//#define HAS_SENSOR_2 1 // comment out if device has user defined sensor #2
//#define HAS_SENSOR_3 1 // comment out if device has user defined sensor #3

//#define BOARD_HAS_PSRAM // use if board has external SPIRAM, note: this will reduce IRAM0 by 64KB for SPIRAM cache
#define DISABLE_BROWNOUT 1 // comment out if you want to keep brownout feature

// on board LED, GPIO2 on most classic ESP32 devkits
// set to NOT_A_PIN if your board has no (usable) LED
#define HAS_LED (2)
//#define HAS_BUTTON (0)  // on board button, boot button on GPIO0
//#define BUTTON_PULLUP 1

// No battery voltage divider on a plain devkit, so skip the ADC probe:
//#define BAT_MEASURE_ADC ADC1_GPIO35_CHANNEL // battery probe GPIO pin -> ADC1_CHANNEL_7
//#define BAT_VOLTAGE_DIVIDER 2 // voltage divider 100k/100k on board

//#define RGB_LED_COUNT 1 // we have 1 LED
//#define HAS_RGB_LED FastLED.addLeds<WS2812, GPIO_NUM_0, GRB>(leds, RGB_LED_COUNT);

// GPS settings
//#define HAS_GPS 1 // use on board GPS
//#define GPS_SERIAL 9600, SERIAL_8N1, GPIO_NUM_12, GPIO_NUM_15 // UBlox NEO 6M RX, TX
//#define GPS_INT GPIO_NUM_13 // 30ns accurary timepulse, to be external wired on pcb: NEO 6M Pin#3 -> GPIO13

// Pins for I2C interface of OLED Display
//#define MY_DISPLAY_SDA (4)
//#define MY_DISPLAY_SCL (15)
//#define MY_DISPLAY_RST (16)

// Settings for on board DS3231 RTC chip
//#define HAS_RTC MY_DISPLAY_SDA, MY_DISPLAY_SCL // SDA, SCL
//#define RTC_INT GPIO_NUM_34 // timepulse with accuracy +/- 2*e-6 [microseconds] = 0,1728sec / day

// Settings for IF482 interface
//#define HAS_IF482 9600, SERIAL_7E1, GPIO_NUM_12, GPIO_NUM_14 // IF482 serial port parameters

// Settings for DCF77 interface
//#define HAS_DCF77 GPIO_NUM_1
//#define DCF77_ACTIVE_LOW 1

#endif