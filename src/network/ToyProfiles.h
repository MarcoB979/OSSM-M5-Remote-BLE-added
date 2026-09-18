#pragma once

// ToyProfiles.h — novice-friendly runtime toy profiles.
//
// Users (or the web config page at /toys) can add BLE toys without recompiling
// the firmware. Each profile describes one toy: an advertised-name prefix plus
// a single feature (vibrate, rotate, …). Profiles are persisted in NVS so they
// survive reboots and OTA updates.

#include <Arduino.h>
#include <stdint.h>
#include <vector>

#include "ToyHub.h"  // ToyFeatureType

struct ToyProfile {
    String prefix;          // advertised name prefix, e.g. "WP-"
    ToyFeatureType type;    // Vibrate/Rotate/Oscillate/…
    int minValue;
    int maxValue;
    String commandTemplate; // e.g. "Vibrate:%d;"
};

void toyProfilesInit();
int  toyProfilesCount();
bool toyProfilesGet(int index, ToyProfile& out);
int  toyProfilesMatch(const char* name);  // returns profile index or -1
bool toyProfilesAdd(const ToyProfile& p); // upserts on prefix, trims whitespace
bool toyProfilesRemove(int index);
