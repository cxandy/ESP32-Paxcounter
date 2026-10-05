#ifndef _WIFICONFIG_H
#define _WIFICONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <Arduino.h>

// Try to connect to stored Wi-Fi credentials, or launch captive portal if
// unavailable. If no one configures within timeoutSeconds, fall back to
// offline mode (no NTP/time sync via Wi-Fi).
bool wifi_config_auto_connect(uint32_t timeoutSeconds);

// Returns true if Wi-Fi is connected and time was synchronized via NTP.
// Should be checked after wifi_config_auto_connect().
bool wifi_time_is_synced(void);

// Get a short status string to show on the matrix display (e.g. "NO NT" or "").
const char *wifi_time_status_str(void);

void wifi_config_stop_portal_if_running(void);

#ifdef __cplusplus
}
#endif

#endif
