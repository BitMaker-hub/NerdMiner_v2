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
#elif defined(LILYGO_S3_T_EMBED)
#include "lilygoS3TEmbed.h"
#elif defined(ESP32_2432S028R)
#include "esp322432s028r.h"
#elif defined(ESP32_2432S028_2USB) // For another type of ESP32_2432S028 version with 2 USB connectors
#include "esp322432s028r.h"
#elif defined(NERMINER_T_QT)
#include "lilygoT_QT.h"
#elif defined(NERDMINER_T_DISPLAY_V1)
#include "lilygoV1TDisplay.h"
#elif defined(ESP32_CAM)
#include "esp32CAM.h"
#elif defined(ESP32RGB)
#include "esp32RGB.h"
#elif defined(M5_STAMP_S3)
#include "m5StampS3.h"
#elif defined(DEVKITV1RGB)
#include "esp32DevKitRGB.h"
#elif defined(S3MINIWEMOS)
#include "esp32S3MiniWemos.h"
#elif defined(S3MINIWEACT)
#include "esp32S3MiniWeact.h"
#elif defined(M5STACK_BOARD)
#include "m5stack.h"
#elif defined(WT32_BOARD)
#include "wt32.h"
#elif defined(NERMINER_S3_GEEK)
#include "waveshareS3Geek.h"
#elif defined(NERDMINER_T_HMI)
#include "lilygoT_HMI.h"
#elif defined(ESP32_SSD1306)
#include "esp32_ssd1306.h"


#else
#error "No device defined"
#endif

#endif // __DEVICE_H__
