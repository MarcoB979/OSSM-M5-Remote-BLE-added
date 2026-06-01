#include "FistComm.h"

#include <esp_now.h>
#include <cstring>

#include "../config/debug.h"
#include "../config/config_ids.h"

namespace {

static bool s_fistPaired = false;
static uint32_t s_fistLastSeenMs = 0;
static uint8_t s_fistAddress[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
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

  if (memcmp(s_fistAddress, mac, 6) == 0 && s_fistPaired) {
    return;
  }

  if (!isBroadcast(s_fistAddress) && !isAllZero(s_fistAddress) && esp_now_is_peer_exist(s_fistAddress)) {
    esp_now_del_peer(s_fistAddress);
  }

  memcpy(s_fistAddress, mac, 6);
  if (ensurePeer(s_fistAddress)) {
    s_fistPaired = true;
    LogDebugFormatted("FIST paired MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                      s_fistAddress[0], s_fistAddress[1], s_fistAddress[2],
                      s_fistAddress[3], s_fistAddress[4], s_fistAddress[5]);
  }
}

static void refreshState() {
  const uint32_t now = millis();
  if (s_fistPaired && (now - s_fistLastSeenMs) > 6000UL) {
    s_fistPaired = false;
  }
}

}  // namespace

bool fistCommIsFrame(const struct_message& msg) {
  const int announcedId = (int)(msg.esp_value + (msg.esp_value >= 0 ? 0.5f : -0.5f));
  return (msg.esp_target == FIST_ID ||
          msg.esp_command == FIST_SPEED ||
          msg.esp_command == FIST_ROTATION ||
          msg.esp_command == FIST_PAUSE ||
          msg.esp_command == FIST_ACCEL ||
          ((msg.esp_command == CONNECT || msg.esp_command == CONN || msg.esp_command == HEARTBEAT) &&
           announcedId == FIST_ID));
}

void fistCommHandleFrame(const uint8_t* mac, const struct_message& msg) {
  (void)msg;
  setPairedAddress(mac);
  s_fistPaired = true;
  s_fistLastSeenMs = millis();
}

bool fistCommIsConnected() {
  refreshState();
  return s_fistPaired;
}

const uint8_t* fistCommGetTxAddress() {
  refreshState();
  return s_fistPaired ? s_fistAddress : s_broadcastAddr;
}

bool fistCommEnsureTxPeer() {
  refreshState();
  return ensurePeer(fistCommGetTxAddress());
}
