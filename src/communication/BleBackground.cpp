#include "BleBackground.h"

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include "BleComm.h"  // bleRadioIsSuspended
#include "../addons/Coyote.h"
#include "../addons/Eject.h"
#include "../addons/FistIT.h"
#include "../network/ToyHub.h"

namespace {

struct Job {
    BleBgJob kind;
    uint32_t arg;
};

QueueHandle_t s_queue = nullptr;
TaskHandle_t s_task = nullptr;
SemaphoreHandle_t s_scanMutex = nullptr;

void runJob(const Job& job) {
    switch (job.kind) {
        case BleBgJob::EjectProbe:
            (void)EjectTryConnectBackground();
            break;
        case BleBgJob::FistITProbe:
            (void)FistITTryConnectBackground();
            break;
        case BleBgJob::CoyoteProbe:
            (void)CoyoteTryConnectBackground();
            break;
        case BleBgJob::ToyScan:
            toyHubScanAndConnectOnce(job.arg);
            break;
    }
}

void taskFn(void*) {
    Job job;
    for (;;) {
        if (xQueueReceive(s_queue, &job, portMAX_DELAY) != pdTRUE) continue;
        // The WiFi portal owns the radio/RAM while open; drop stale jobs.
        if (bleRadioIsSuspended()) continue;
        // Don't fight the OSSM connect path over the shared scanner.
        BleScanGuard guard;
        if (!guard) {
            vTaskDelay(pdMS_TO_TICKS(200));  // scanner busy; retry on a later enqueue
            continue;
        }
        runJob(job);
    }
}

}  // namespace

void bleBackgroundInit() {
    if (s_task) return;
    s_queue = xQueueCreate(8, sizeof(Job));
    s_scanMutex = xSemaphoreCreateMutex();
    if (!s_queue || !s_scanMutex) return;
    xTaskCreatePinnedToCore(taskFn, "bleBg", 6144, nullptr, 2, &s_task, 1);
}

bool bleBackgroundRequest(BleBgJob job, uint32_t arg) {
    if (!s_queue) return false;
    Job j{job, arg};
    return xQueueSend(s_queue, &j, 0) == pdTRUE;
}

bool bleScanTryEnter() {
    if (!s_scanMutex) return true;  // not initialised yet; proceed unguarded
    return xSemaphoreTake(s_scanMutex, 0) == pdTRUE;
}

void bleScanExit() {
    if (s_scanMutex) xSemaphoreGive(s_scanMutex);
}
