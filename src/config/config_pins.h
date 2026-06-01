#pragma once
// ---------------------------------------------------------------------------
// config_pins.h
// Pin assignments and encoder range – safe to include from any translation unit.
// Object definitions (OneButton, etc.) stay in config.h / main.cpp.
// ---------------------------------------------------------------------------

#ifdef ARDUINO_M5STACK_CORES3
// Encoder pin definitions – CoreS3
#define ENC_1_CLK 5
#define ENC_1_DT  9

#define ENC_2_CLK 18
#define ENC_2_DT  17

#define ENC_3_CLK 1
#define ENC_3_DT  2

#define ENC_4_CLK 7
#define ENC_4_DT  6
#else
// Encoder pin definitions – Core2
#define ENC_1_CLK 25
#define ENC_1_DT  26

#define ENC_2_CLK 13
#define ENC_2_DT  14

#define ENC_3_CLK 33
#define ENC_3_DT  32

#define ENC_4_CLK 19
#define ENC_4_DT  27
#endif

#define Encoder_MAP 300
