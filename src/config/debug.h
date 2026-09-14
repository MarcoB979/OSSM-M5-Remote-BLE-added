#pragma once
#include <Arduino.h>

// Uncomment to enable debug output over Serial
#define DEBUG

// Uncomment to stream raw OSSM rail position every BLE poll cycle (very
// verbose — only for verifying live position telemetry, e.g. for estim sync).
#define DEBUG_POSITION_STREAM

// Uncomment to print the confirmed OSSM setpoint + live rail telemetry
// once per second (verify the individual characteristics are received).
#define SHOW_TELEMETRY

#ifdef DEBUG
  #define LogDebug(...)          Serial.println(__VA_ARGS__)
  #define LogDebugFormatted(...) Serial.printf(__VA_ARGS__)
#else
  #define LogDebug(...)          ((void)0)
  #define LogDebugFormatted(...) ((void)0)
#endif
