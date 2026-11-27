#include "timezones.h"

const TzOption TZ_OPTIONS[] = {
    { "UTC", "UTC0" },
    { "London, Lisbon, Canary Islands", "GMT0BST,M3.5.0/1,M10.5.0" },
    { "Madrid, Paris, Berlin, Rome (CET)", "CET-1CEST,M3.5.0,M10.5.0/3" },
    { "Athens, Helsinki, Kyiv (EET)", "EET-2EEST,M3.5.0/3,M10.5.0/4" },
    { "Moscow, Istanbul", "<+03>-3" },
    { "Dubai", "<+04>-4" },
    { "India", "IST-5:30" },
    { "China, Singapore, Perth", "<+08>-8" },
    { "Tokyo, Seoul", "<+09>-9" },
    { "Sydney, Melbourne", "AEST-10AEDT,M10.1.0,M4.1.0/3" },
    { "Auckland", "NZST-12NZDT,M9.5.0,M4.1.0/3" },
    { "Honolulu", "HST10" },
    { "Los Angeles (Pacific)", "PST8PDT,M3.2.0,M11.1.0" },
    { "Denver (Mountain)", "MST7MDT,M3.2.0,M11.1.0" },
    { "Phoenix", "MST7" },
    { "Chicago (Central)", "CST6CDT,M3.2.0,M11.1.0" },
    { "Mexico City", "CST6" },
    { "New York (Eastern)", "EST5EDT,M3.2.0,M11.1.0" },
    { "Bogota, Lima", "<-05>5" },
    { "Buenos Aires, Sao Paulo", "<-03>3" },
};

const size_t TZ_OPTIONS_COUNT = sizeof(TZ_OPTIONS) / sizeof(TZ_OPTIONS[0]);

String effectiveTzRule(const TSettings& s)
{
    if (s.TzRule.length() > 0) return s.TzRule;
    // POSIX sign is inverted: UTC+2 is "UTC-2"
    int offset = -s.Timezone;
    if (offset == 0) return "UTC0";
    return String("UTC") + (offset > 0 ? "+" : "") + String(offset);
}
