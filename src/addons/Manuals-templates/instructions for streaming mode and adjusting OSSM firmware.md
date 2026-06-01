To be able to use the M5 remote with OSSM to set the OSSM in streaming mode,
the OSSM firmware must be able to accept 2 simultanious BLE connections.

A few small adjustments in the firmware are necessary:

plarformio.ini file:
Under [common] add a general build flag:
build_flags =
    -fexceptions
    -D CONFIG_BT_NIMBLE_MAX_CONNECTIONS=2

Also, buildunflags should be changed by adding
     -fno-exceptions

The -fexceptions / fno-exceptions flags are not specifically there for BLE, but were a fix to get a successfull build. The added max_connections flag is the actual one to allow 2 connections.

The beginning of the [common] part in platformio should now look like this:

[common]
board_build.partitions = min_spiffs.csv
build_flags =
    -fexceptions
    -D CONFIG_BT_NIMBLE_MAX_CONNECTIONS=2
build_unflags =
    -std=gnu++11
    -fno-exceptions

In nimble.cpp:
add the following lines of code in the call ServerCallbacks : public NolBLEServerCallbacks:

        // Keep advertising if we can accept more connections
        if (pServer->getConnectedCount() < CONFIG_BT_NIMBLE_MAX_CONNECTIONS) {
            pServer->startAdvertising();
            ESP_LOGI(NIMBLE_TAG, "Started advertising again.");
        }

the beginning of that callback now should look like this:

class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
        ESP_LOGI(NIMBLE_TAG, "Client connected: %s",
                 connInfo.getAddress().toString().c_str());
        ESP_LOGI(NIMBLE_TAG, "Connection count: %d",
                 pServer->getConnectedCount());

        // Keep advertising if we can accept more connections
        if (pServer->getConnectedCount() < CONFIG_BT_NIMBLE_MAX_CONNECTIONS) {
            pServer->startAdvertising();
            ESP_LOGI(NIMBLE_TAG, "Started advertising again.");
        }
