#pragma once
#include <Arduino.h>

// Uncomment to enable debug output over Serial
#define DEBUG

#ifdef DEBUG
  #define LogDebug(...)          Serial.println(__VA_ARGS__)
  #define LogDebugFormatted(...) Serial.printf(__VA_ARGS__)
#else
  #define LogDebug(...)          ((void)0)
  #define LogDebugFormatted(...) ((void)0)
#endif
