#include "EjectComm.h"

#include <esp_now.h>
#include <cstring>

#include "../debug.h"
#include "../config_ids.h"

namespace {

static bool s_ejectPaired = false;
static uint32_t s_ejectLastSeenMs = 0;
static uint8_t s_ejectAddress[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static const uint8_t s_broadcastAddr[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

static bool isBroadcast(const uint8_t *addr) {
  return addr &&
         addr[0] == 0xFF && addr[1] == 0xFF && addr[2] == 0xFF &&
         addr[3] == 0xFF && addr[4] == 0xFF && addr[5] == 0xFF;
}

static bool isAllZero(const uint8_t *addr) {
  return addr &&
         addr[0] == 0x00 && addr[1] == 0x00 && addr[2] == 0x00 &&
         addr[3] == 0x00 && addr[4] == 0x00 && addr[5] == 0x00;
}

static bool ensurePeer(const uint8_t *addr) {
  if (!addr) return false;
  if (isBroadcast(addr) || isAllZero(addr)) return true;
  if (esp_now_is_peer_exist(addr)) return true;

  esp_now_peer_info_t peerInfoLocal = {};
  memcpy(peerInfoLocal.peer_addr, addr, 6);
  peerInfoLocal.channel = 0;
  peerInfoLocal.encrypt = false;
  return esp_now_add_peer(&peerInfoLocal) == ESP_OK;
}

static void setPairedAddress(const uint8_t *mac) {
  if (!mac) return;
  if (isBroadcast(mac) || isAllZero(mac)) return;

  if (memcmp(s_ejectAddress, mac, 6) == 0 && s_ejectPaired) {
    return;
  }

  if (!isBroadcast(s_ejectAddress) && !isAllZero(s_ejectAddress) && esp_now_is_peer_exist(s_ejectAddress)) {
    esp_now_del_peer(s_ejectAddress);
  }

  memcpy(s_ejectAddress, mac, 6);
  if (ensurePeer(s_ejectAddress)) {
    s_ejectPaired = true;
    LogDebugFormatted("EJECT paired MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                      s_ejectAddress[0], s_ejectAddress[1], s_ejectAddress[2],
                      s_ejectAddress[3], s_ejectAddress[4], s_ejectAddress[5]);
  }
}

static void refreshState() {
  const uint32_t now = millis();
  if (s_ejectPaired && (now - s_ejectLastSeenMs) > 6000UL) {
    s_ejectPaired = false;
  }
}

}  // namespace

bool ejectCommIsFrame(const struct_message& msg) {
  const int announcedId = (int)(msg.esp_value + (msg.esp_value >= 0 ? 0.5f : -0.5f));
  return (msg.esp_target == CUM ||
          msg.esp_target == CUM_ID ||
          msg.esp_command == CUMSPEED ||
          msg.esp_command == CUMTIME ||
          msg.esp_command == CUMSIZE ||
          msg.esp_command == CUMACCEL ||
          ((msg.esp_command == CONNECT || msg.esp_command == CONN || msg.esp_command == HEARTBEAT) &&
           announcedId == CUM_ID));
}

void ejectCommHandleFrame(const uint8_t* mac, const struct_message& msg) {
  (void)msg;
  setPairedAddress(mac);
  s_ejectPaired = true;
  s_ejectLastSeenMs = millis();
}

bool ejectCommIsConnected() {
  refreshState();
  return s_ejectPaired;
}

const uint8_t* ejectCommGetTxAddress() {
  refreshState();
  return s_ejectPaired ? s_ejectAddress : s_broadcastAddr;
}

bool ejectCommEnsureTxPeer() {
  refreshState();
  return ensurePeer(ejectCommGetTxAddress());
}
