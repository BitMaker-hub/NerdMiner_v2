#ifndef API_H
#define API_H

#include <WebServer.h>
#include <ArduinoJson.h>
#include "wManager.h"
#include "drivers/storage/nvMemory.h"

void setupAPI();
void api_loop();

#endif // API_H
