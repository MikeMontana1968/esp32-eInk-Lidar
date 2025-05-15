/*
Set Board Type to: Vision Master E290
https://github.com/todd-herbert/heltec-eink-modules/blob/main/docs/WirelessPaper/wireless_paper.md

*/

#include "setup.h" // Tank size, refresh rates etc
#include "task.h"
#include "vlx_sampler.h"
#include "eink_display.h"

VlxTask *vlxTask;
EinkDisplayTask *einkTask;

void setup() {
	Serial.begin(115200);
    Serial.print("Running ");
    Serial.print(" Version: ");
    Serial.println(__TIMESTAMP__);
    delay(100);
    


    Serial.println("Allocate Tasks"); delay(50);
        vlxTask = new VlxTask();
        einkTask = new EinkDisplayTask();
        
    Serial.println("Start Task VLX"); delay(50);  vlxTask->start();
    Serial.println("Start Task eInk"); delay(50); einkTask->start();
    Serial.println("Setup Complete");
}

void loop() {
    Serial.println("loop()");
    delay(2 * 1000);
}