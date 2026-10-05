#ifdef HAS_SMARTMATRIX

// 64x32 HUB75 RGB panel driven by SmartMatrix4-ESP32 (I2S LCD mode + DMA).
//
// Layout of the panel:
//
//   y  0..4   header, 3x5 font: "PAX" left, "Wnn Bnn" right aligned
//   y  8..22  current pax count, 3x5 digits scaled 3x -> 9x15 px
//   y 25..31  trend graph, one 2px bar per count change
//
// The refresh itself is done by the I2S DMA engine, so this module only has to
// repaint the framebuffer. A bar is pushed whenever libpax reports a new count,
// which makes the trend self-clocking - no extra timer needed.

#include "globals.h"
#include "smartmatrixdisplay.h"
#ifdef HAS_SMARTMATRIX || defined(HAS_MATRIX_DISPLAY)
#include "wificonfig.h"
#endif
// SmartMatrix buffer / layer configuration for the 64x32 panel
#define SM_COLOR_DEPTH    24 // layers store rgb24 directly
#define SM_REFRESH_DEPTH  36 // 24 / 36 / 48 - higher is smoother but costlier
#define SM_DMA_ROWS       4  // unused on ESP32, keep at the default
#define SM_PANEL_TYPE     SM_PANELTYPE_HUB75_32ROW_MOD16SCAN
#define SM_MATRIX_OPTIONS (SM_HUB75_OPTIONS_ESP32_INVERT_CLK)

SMARTMATRIX_ALLOCATE_BUFFERS(smMatrix, LED_MATRIX_WIDTH, LED_MATRIX_HEIGHT,
                             SM_REFRESH_DEPTH, SM_DMA_ROWS, SM_PANEL_TYPE,
                             SM_MATRIX_OPTIONS);
SMARTMATRIX_ALLOCATE_BACKGROUND_LAYER(smLayer, LED_MATRIX_WIDTH,
                                      LED_MATRIX_HEIGHT, SM_COLOR_DEPTH,
                                      SM_BACKGROUND_OPTIONS_NONE);

// layout
#define SM_HEADER_Y (0)
#define SM_DIGIT_Y  (8)
#define SM_DIGIT_SCALE 3
#define SM_TREND_Y  (25)
#define SM_TREND_H  (7)

// repaint rate of the task below; the DMA refresh runs far faster than this
#define SM_REFRESH_MS (500)

TaskHandle_t smDisplayTask = NULL;
bool smDisplayIsOn = false;

// rolling history of pax counts for the trend graph
static uint16_t smTrend[MATRIX_DISPLAY_TREND_LEN];
static uint8_t smTrendPos = 0;
static uint8_t smTrendCount = 0;
static uint16_t smLastPax = 0;
static bool smFirstSample = true;

// 3x5 digit font, one byte per row, bit 2 is the leftmost column
static const uint8_t SM_DIGIT_3X5[10][5] = {
    {0x07, 0x05, 0x05, 0x05, 0x07}, // 0
    {0x02, 0x06, 0x02, 0x02, 0x07}, // 1
    {0x07, 0x01, 0x07, 0x04, 0x07}, // 2
    {0x07, 0x01, 0x07, 0x01, 0x07}, // 3
    {0x05, 0x05, 0x07, 0x01, 0x01}, // 4
    {0x07, 0x04, 0x07, 0x01, 0x07}, // 5
    {0x07, 0x04, 0x07, 0x05, 0x07}, // 6
    {0x07, 0x01, 0x01, 0x01, 0x01}, // 7
    {0x07, 0x05, 0x07, 0x05, 0x07}, // 8
    {0x07, 0x05, 0x07, 0x01, 0x07}, // 9
};

// draw one digit as a scale x scale block grid, returns the x advance
static int16_t drawBigDigit(uint8_t digit, int16_t x, int16_t y, int16_t scale,
                            const rgb24 &color) {
  for (uint8_t row = 0; row < 5; row++)
    for (uint8_t col = 0; col < 3; col++)
      if (SM_DIGIT_3X5[digit][row] & (1 << (2 - col)))
        smLayer.fillRectangle(x + col * scale, y + row * scale,
                              x + col * scale + scale - 1,
                              y + row * scale + scale - 1, color);
  return 3 * scale + scale; // glyph plus a one-dot gap
}

// right align a short string in the 3x5 font
static void drawStringRight(int16_t y, const char *text,
                            const rgb24 &color) {
  int16_t width = (int16_t)(strlen(text) * 4); // 3px glyph + 1px advance
  smLayer.drawString(LED_MATRIX_WIDTH - width, y, color, text);
}

void sm_display_init(void) {
  ESP_LOGI(TAG, "Initializing SmartMatrix %dx%d panel", LED_MATRIX_WIDTH,
           LED_MATRIX_HEIGHT);

  smMatrix.addLayer(&smLayer);
  smMatrix.begin();

  smLayer.setFont(font3x5);
  smLayer.setBrightness(MATRIX_DISPLAY_BRIGHTNESS);
  smLayer.enableColorCorrection(false);
  smLayer.fillScreen(rgb24(0, 0, 0));
  smLayer.swapBuffers(false);

  smDisplayIsOn = true;

  ESP_LOGI(TAG, "SmartMatrix panel started, DMA refresh running");
}

void sm_display_refresh(void) {
  struct count_payload_t count;
  libpax_counter_count(&count);

  // a changed count means a new send cycle, so push a trend bar
  if (smFirstSample || (count.pax != smLastPax)) {
    smTrend[smTrendPos] = count.pax;
    smTrendPos = (smTrendPos + 1) % MATRIX_DISPLAY_TREND_LEN;
    if (smTrendCount < MATRIX_DISPLAY_TREND_LEN)
      smTrendCount++;
    smLastPax = count.pax;
    smFirstSample = false;
  }

  const rgb24 colorOff = rgb24(0, 0, 0);
  const rgb24 colorPax = rgb24(0, 255, 0);
  const rgb24 colorDim = rgb24(0, 110, 0);
  const rgb24 colorWifi = rgb24(0, 170, 220);
  const rgb24 colorBle = rgb24(200, 60, 180);
  const rgb24 colorTrend = rgb24(0, 190, 90);

  // follow the on/off state driven by the button or a remote command
  if (smDisplayIsOn != cfg.screenon) {
    smDisplayIsOn = cfg.screenon;
    if (!smDisplayIsOn) {
      smLayer.fillScreen(colorOff);
      smLayer.swapBuffers(false);
      return;
    }
  }
  if (!smDisplayIsOn)
    return;

  smLayer.fillScreen(colorOff);

  // header: label on the left, wifi/ble split on the right
  char header[12];
  smLayer.drawString(0, SM_HEADER_Y, colorDim, "PAX");
  const char *tstat = wifi_time_status_str();
  snprintf(header, sizeof(header), "%sW%u B%u", tstat, count.wifi_count,
           count.ble_count);
  drawStringRight(SM_HEADER_Y, header, count.ble_count ? colorBle : colorWifi);

  // current pax count, drawn left to right and clipped at the panel edge
  char paxText[6];
  snprintf(paxText, sizeof(paxText), "%u", (unsigned int)count.pax);
  int16_t digitX = 2;
  for (const char *p = paxText; *p; p++) {
    if (digitX + 3 * SM_DIGIT_SCALE > LED_MATRIX_WIDTH)
      break;
    if (*p >= '0' && *p <= '9')
      digitX += drawBigDigit(*p - '0', digitX, SM_DIGIT_Y, SM_DIGIT_SCALE,
                             colorPax);
    else
      digitX += SM_DIGIT_SCALE;
  }

  // trend graph, scaled against the busiest sample in the window
  uint16_t peak = 1;
  for (uint8_t i = 0; i < smTrendCount; i++)
    if (smTrend[i] > peak)
      peak = smTrend[i];

  const int16_t barWidth = LED_MATRIX_WIDTH / MATRIX_DISPLAY_TREND_LEN;
  for (uint8_t i = 0; i < smTrendCount; i++) {
    // oldest sample left, newest right
    uint8_t idx = (smTrendPos + i + MATRIX_DISPLAY_TREND_LEN -
                   smTrendCount) % MATRIX_DISPLAY_TREND_LEN;
    int16_t height = (int16_t)((uint32_t)smTrend[idx] * SM_TREND_H / peak);
    if (height > 0)
      smLayer.fillRectangle(i * barWidth, SM_TREND_Y + SM_TREND_H - height,
                            (i + 1) * barWidth - 1, SM_TREND_Y + SM_TREND_H - 1,
                            colorTrend);
  }

  smLayer.swapBuffers(false);
}

void sm_display_task(void *parameter) {
  (void)parameter;
  while (1) {
    sm_display_refresh();
    vTaskDelay(SM_REFRESH_MS / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}

#endif // HAS_SMARTMATRIX