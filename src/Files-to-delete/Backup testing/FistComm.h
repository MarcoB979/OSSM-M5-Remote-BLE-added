#pragma once

#include <stdint.h>
#include "EspNowComm.h"

bool fistCommIsFrame(const struct_message& msg);
void fistCommHandleFrame(const uint8_t* mac, const struct_message& msg);
bool fistCommIsConnected();
const uint8_t* fistCommGetTxAddress();
bool fistCommEnsureTxPeer();
