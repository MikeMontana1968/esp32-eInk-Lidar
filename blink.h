#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "string.h"
#include "task.h"

class BlinkTask : public Task {
public:
    BlinkTask() : Task("BlinkTask", 2048, 3), nBlink(0) {
      pinMode(ONBOARD_LED, OUTPUT);
    }

protected:
    void run() override {
        while (true) {
            nBlink++;
            Serial.printf("BlinkTask= %d\n", nBlink);
            digitalWrite(ONBOARD_LED, nBlink % 1);
            vTaskDelay(1000 / portTICK_PERIOD_MS);  // Delay for 1 second
        }
    }

private:
    int nBlink;
    const int ONBOARD_LED = 2;
};