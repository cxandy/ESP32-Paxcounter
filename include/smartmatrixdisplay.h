#ifndef _SMARTMATRIXDISPLAY_H
#define _SMARTMATRIXDISPLAY_H

#ifdef HAS_SMARTMATRIX

#include "globals.h"
#include "configmanager.h"
#include <libpax_api.h>

// SmartMatrix needs the hardware pinout selected before the driver header is
// pulled in. AZSMZ_ESP32Matrix_v15 is the v1.5 controller board.
#define GPIOPINOUT AZSMZ_ESP32Matrix_v15
#include <MatrixHardware_ESP32_V0.h>
#include <SmartMatrix.h>

extern TaskHandle_t smDisplayTask;
extern bool smDisplayIsOn;

// Bring up the panel. Must be called before the sniffing tasks start, so the
// DMA refresh owns its buffers from the very beginning.
void sm_display_init(void);

// Repaint the panel from the current libpax counters. Cheap enough to call a
// few times per second, but not from an ISR.
void sm_display_refresh(void);

// The panel's own refresh task, pinned to core 0 to keep core 1 free for
// WiFi/BLE sniffing.
void sm_display_task(void *parameter);

#endif // HAS_SMARTMATRIX

#endif // _SMARTMATRIXDISPLAY_H