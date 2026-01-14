/*
Set Board Type to: Vision Master E290
https://github.com/todd-herbert/heltec-eink-modules/blob/main/docs/WirelessPaper/wireless_paper.md

*/
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE

#include "VLXTask.cpp"
#include "EinkDisplay.cpp"
#define I2C_SDA GPIO_NUM_39
#define I2C_SCL GPIO_NUM_38
#define TAG_MAIN "main"
VlxTask *vlxTask;
EinkDisplayTask *einkTask;
	// esp_log_level_set("*", ESP_LOG_NONE);					//<<<Default logging level for all tags
	// esp_log_level_set("OneOfMyTagNames", ESP_LOG_VERBOSE);
	// esp_log_level_set("AnotherOfMyTagNames", ESP_LOG_WARN);
void setup() {
    ESP_LOGE(TAG_MAIN, "Setup!");
	Serial.begin(115200);
    Serial.print("Running ");
    Serial.print(" Version: ");
    Serial.println(TAG_MAIN);
    delay(100);
    
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
