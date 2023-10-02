#ifndef _NVMEMORY_H_
#define _NVMEMORY_H_

// we only have one implementation right now and nothing to choose from.
#define NVMEM_SPIFFS

#include "../devices/device.h"
#include "storage.h"

// Handles load and store of user settings, except wifi credentials. Those are managed by the wifimanager.
class nvMemory
{
public: 
    nvMemory();
    ~nvMemory();
    bool saveConfig(TSettings* Settings);
    bool loadConfig(TSettings* Settings);
    bool deleteConfig();
private:
    bool init();
    bool Initialized_;
};

#ifndef NVMEM_SPIFFS
#error We need some kind of permanent storage implementation!
#endif //NVMEM_TYPE

#endif // _NVMEMORY_H_
