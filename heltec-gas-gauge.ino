/*
Set Board Type to: Vision Master E290
https://github.com/todd-herbert/heltec-eink-modules/blob/main/docs/WirelessPaper/wireless_paper.md

*/
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE

#include "vlx_sampler.cpp"
#include "eink_display.cpp"

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

        vlxTask = new VlxTask();
        einkTask = new EinkDisplayTask(vlxTask);

    vlxTask->start();
    einkTask->start();
    ESP_LOGE(TAG_MAIN,"Setup Complete");
}

void loop() {
    ESP_LOGV(TAG_MAIN, ".");
    vTaskDelay(10000 / portTICK_PERIOD_MS);
}