#pragma once

#include "config/config_ids.h"

// Old firmware addon modules used this channel constant.
#ifndef ESP_NOW_CHANNEL
#define ESP_NOW_CHANNEL 1
#endif

#ifndef CONN
#define CONN         0
#endif
#ifndef SPEED
#define SPEED        1
#endif
#ifndef DEPTH
#define DEPTH        2
#endif
#ifndef STROKE
#define STROKE       3
#endif
#ifndef SENSATION
#define SENSATION    4
#endif
#ifndef PATTERN
#define PATTERN      5
#endif
#ifndef TORQE_F
#define TORQE_F      6
#endif
#ifndef TORQE_R
#define TORQE_R      7
#endif
#ifndef OFF
#define OFF          10
#endif
#ifndef ON
#define ON           11
#endif
#ifndef SETUP_D_I
#define SETUP_D_I    12
#endif
#ifndef SETUP_D_I_F
#define SETUP_D_I_F  13
#endif
#ifndef REBOOT
#define REBOOT       14
#endif
#ifndef CUMSPEED
#define CUMSPEED     20
#endif
#ifndef CUMTIME
#define CUMTIME      21
#endif
#ifndef CUMSIZE
#define CUMSIZE      22
#endif
#ifndef CUMACCEL
#define CUMACCEL     23
#endif
#ifndef FIST_SPEED
#define FIST_SPEED   30
#endif
#ifndef FIST_ROTATION
#define FIST_ROTATION 31
#endif
#ifndef FIST_PAUSE
#define FIST_PAUSE   32
#endif
#ifndef FIST_ACCEL
#define FIST_ACCEL   33
#endif
#ifndef CONNECT
#define CONNECT      88
#endif
#ifndef HEARTBEAT
#define HEARTBEAT    99
#endif

