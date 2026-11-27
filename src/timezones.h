#ifndef _TIMEZONES_H_
#define _TIMEZONES_H_

#include <Arduino.h>
#include "drivers/storage/storage.h"

struct TzOption {
    const char* label;
    const char* rule; // POSIX TZ string
};

extern const TzOption TZ_OPTIONS[];
extern const size_t TZ_OPTIONS_COUNT;

// Returns Settings.TzRule if set, otherwise a fixed POSIX rule built from the legacy integer Timezone.
String effectiveTzRule(const TSettings& s);

#endif // _TIMEZONES_H_
