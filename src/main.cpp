/*
Set Board Type to: Vision Master E290
https://github.com/todd-herbert/heltec-eink-modules/blob/main/docs/WirelessPaper/wireless_paper.md

*/
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#define I2C_SDA GPIO_NUM_39
#define I2C_SCL GPIO_NUM_38
#define TAG_MAIN "main"

#include "classes/VlxTask.cpp"
#include "classes/EinkDisplayTask.cpp"

VlxTask *vlxTask;
EinkDisplayTask *einkTask;

void setup() {
    ESP_LOGE(TAG_MAIN, "Setup!");
	Serial.begin(115200);
    Serial.print("Running Version: ");
    Serial.println(TAG_MAIN);
    
    vlxTask = new VlxTask(I2C_SDA, I2C_SCL);
    einkTask = new EinkDisplayTask(I2C_SDA, I2C_SCL, vlxTask);

    vlxTask->start();
    einkTask->start();
    ESP_LOGE(TAG_MAIN,"Setup Complete");
}

void loop() {
    ESP_LOGV(TAG_MAIN, ".");
    vTaskDelay(10000 / portTICK_PERIOD_MS);
}