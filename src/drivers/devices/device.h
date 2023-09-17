#ifndef __DEVICE_H__
#define __DEVICE_H__

#if defined(NERDMINERV2)
#include "devices/nerdMinerV2.h"
#elif defined(DEVKITV1)
#include "devices/esp32DevKit.h"
#elif defined(TDISPLAY)
#include "devices/lilygoS3TDisplay.h"
#elif defined(NERMINER_S3_AMOLED)
#include "devices/lilygoS3Amoled.h"
#elif defined(NERMINER_S3_DONGLE)
#include "devices/lilygoS3Dongle.h"
#elif defined(ESP32_2432S028R)
#include "devices/esp322432s028r.h"
#elif defined(NERMINER_T_QT)
#include "devices/lilygoT_QT.h"
#elif defined(NERDMINER_T_DISPLAY_V1)
#include "devices/lilygoV1TDisplay.h"
#elif defined(ESP32_CAM)
#include "esp32CAM.h"

#else
#error "No device defined"
#endif

#endif // __DEVICE_H__