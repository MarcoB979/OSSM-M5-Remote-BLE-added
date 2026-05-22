#pragma once

#include <stdint.h>
#include "EspNowComm.h"

bool ejectCommIsFrame(const struct_message& msg);
void ejectCommHandleFrame(const uint8_t* mac, const struct_message& msg);
bool ejectCommIsConnected();
const uint8_t* ejectCommGetTxAddress();
bool ejectCommEnsureTxPeer();
