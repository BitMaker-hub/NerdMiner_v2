#ifndef __DEVICE_H__
#define __DEVICE_H__

#if defined(NERDMINERV2)
#include "nerdMinerV2.h"
#elif defined(DEVKITV1)
#include "esp32DevKit.h"
#elif defined(TDISPLAY)
#include "lilygoS3TDisplay.h"
#elif defined(NERMINER_S3_AMOLED)
#include "lilygoS3Amoled.h"
#elif defined(NERMINER_S3_DONGLE)
#include "lilygoS3Dongle.h"
#elif defined(ESP32_CAM)
#include "esp32CAM.h"
#else
#error "No device defined"
#endif

#endif // __DEVICE_H__