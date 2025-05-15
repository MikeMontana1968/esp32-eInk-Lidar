#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
// https://saludpcb.com/learn-c-inheritance-with-esp32-powerful-task-setup/
class Task {
public:
    Task(const char* name, uint32_t stackSize, UBaseType_t priority)
        : taskName(name), taskStackSize(stackSize), taskPriority(priority), taskHandle(nullptr) {}

    void start() {
        xTaskCreate(taskFunctionWrapper, taskName, taskStackSize, this, taskPriority, &taskHandle);
    }

protected:
    virtual void run() = 0;

private:
    static void taskFunctionWrapper(void* parameter) {
        Task* task = static_cast<Task*>(parameter);
        task->run();
    }

    const char* taskName;
    uint32_t taskStackSize;
    UBaseType_t taskPriority;
    TaskHandle_t taskHandle;
};