#include <Arduino.h>
// #include <cstdarg>
// #include <iostream>
#include <LittleFS.h>
#include "bwc_debug.h"

void log2file(const char* s)
{
    LittleFS.begin();
    File file = LittleFS.open(F("debugoutput.txt"), "a");
    if (!file) {
        // Serial.println(F("Failed to save states.txt"));
        return;
    }

    file.printf(s);
    file.close();
}
