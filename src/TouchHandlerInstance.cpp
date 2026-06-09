#include "TouchHandler.h"
#include "drivers/devices/device.h"

// Provide a single global TouchHandler instance for the build.
// The pins and configuration are defined in the device headers.
#ifdef TOUCH_ENABLE
TouchHandler touchHandler(ETOUCH_CS, TOUCH_IRQ, SPI);
#endif
