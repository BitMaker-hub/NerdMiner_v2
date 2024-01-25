#ifndef __DEVICE_H__
#define __DEVICE_H__

#if defined(NERDMINERV2)
#include "nerdMinerV2.h"
#elif defined(M5STICK_C)
#include "M5Stick-C.h"
#elif defined(DEVKITV1)
#include "esp32DevKit.h"
#elif defined(TDISPLAY)
#include "lilygoS3TDisplay.h"
#elif defined(NERMINER_S3_AMOLED)
#include "lilygoS3Amoled.h"
#elif defined(NERMINER_S3_DONGLE)
#include "lilygoS3Dongle.h"
#elif defined(ESP32_2432S028R)
#include "esp322432s028r.h"
#elif defined(NERMINER_T_QT)
#include "lilygoT_QT.h"
#elif defined(NERDMINER_T_DISPLAY_V1)
#include "lilygoV1TDisplay.h"
#elif defined(ESP32_CAM)
#include "esp32CAM.h"
#elif defined(M5_STAMP_S3)
#include "m5StampS3.h"
#elif defined(ESP32RGB)
#include "esp32RGB.h"
#elif defined(NERDMINER_T_HMI)
#include "lilygoT_HMI.h"

#else
#error "No device defined"
#endif

#endif // __DEVICE_H__
