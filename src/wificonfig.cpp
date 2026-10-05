#include "wificonfig.h"
#include "globals.h"
#include "configmanager.h"
#include <WiFiManager.h>
#include <Preferences.h>
#include <time.h>

static bool time_synced = false;
static bool portal_running = false;
static WiFiManager *wm = nullptr;
static TaskHandle_t portal_timeout_task = NULL;

static void portal_timeout_callback(void *parameter) {
  uint32_t timeout = (uint32_t)(uintptr_t)parameter;
  vTaskDelay(timeout * 1000UL / portTICK_PERIOD_MS);
  if (portal_running && wm) {
    ESP_LOGI(TAG, "Wi-Fi config portal timed out after %u seconds, falling back to offline mode", timeout);
    wm->stopConfigPortal();
  }
  portal_timeout_task = NULL;
  vTaskDelete(NULL);
}

bool wifi_time_is_synced(void) {
  return time_synced;
}

const char *wifi_time_status_str(void) {
  return time_synced ? "" : "!T";
}

void wifi_config_stop_portal_if_running(void) {
  if (portal_running && wm) {
    wm->stopConfigPortal();
    portal_running = false;
  }
  if (portal_timeout_task) {
    vTaskDelete(portal_timeout_task);
    portal_timeout_task = NULL;
  }
}

bool wifi_config_auto_connect(uint32_t timeoutSeconds) {
  time_synced = false;
  portal_running = false;

  Preferences prefs;
  prefs.begin("wificfg", true);
  String ssid = prefs.getString("ssid", "");
  String pass = prefs.getString("pass", "");
  prefs.end();

  if (wm == nullptr) {
    wm = new WiFiManager();
  }

  wm->setConfigPortalBlocking(true);
  wm->setConfigPortalTimeout((int)timeoutSeconds);
  wm->setSaveConfigCallback([]() {
    Preferences prefs2;
    prefs2.begin("wificfg", false);
    prefs2.putString("ssid", WiFi.SSID());
    prefs2.putString("pass", WiFi.psk());
    prefs2.end();
    ESP_LOGI(TAG, "Wi-Fi credentials saved to NVS");
  });

  // Try to connect with saved credentials first
  bool connected = false;
  if (ssid.length() > 0) {
    ESP_LOGI(TAG, "Attempting to connect to stored Wi-Fi: %s", ssid.c_str());
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), pass.c_str());
    // give it a short window before falling back to portal
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
      delay(100);
    }
    connected = (WiFi.status() == WL_CONNECTED);
  }

  if (!connected) {
    ESP_LOGI(TAG, "Launching Wi-Fi captive portal with timeout %u seconds", timeoutSeconds);
    portal_running = true;
    connected = wm->autoConnect("Paxcounter-Setup", nullptr);
    portal_running = false;
  }

  if (portal_timeout_task) {
    vTaskDelete(portal_timeout_task);
    portal_timeout_task = NULL;
  }

  if (!connected) {
    ESP_LOGW(TAG, "Wi-Fi not connected, continuing in offline mode");
    WiFi.mode(WIFI_OFF);
    time_synced = false;
    return false;
  }

  ESP_LOGI(TAG, "Wi-Fi connected: %s", WiFi.SSID().c_str());

  // Try to sync time via NTP
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  struct tm timeinfo;
  time_t now = time(nullptr);
  if (now < 8 * 3600 * 2) {
    // wait briefly for NTP sync
    unsigned long t0 = millis();
    while (millis() - t0 < 3000) {
      now = time(nullptr);
      localtime_r(&now, &timeinfo);
      if (now > 8 * 3600 * 2) {
        time_synced = true;
        break;
      }
      delay(50);
    }
  } else {
    time_synced = true;
  }

  if (time_synced) {
    ESP_LOGI(TAG, "Time synchronized via NTP");
  } else {
    ESP_LOGW(TAG, "NTP time sync timed out");
  }

  return connected;
}
