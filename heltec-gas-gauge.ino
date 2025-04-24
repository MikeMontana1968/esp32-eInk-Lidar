/*
Set Board Type to: Vision Master E290
https://github.com/todd-herbert/heltec-eink-modules/blob/main/docs/WirelessPaper/wireless_paper.md

*/

#include <heltec-eink-modules.h>
#include "Fonts/FreeSans9pt7b.h"
#include "Fonts/FreeSansBold9pt7b.h"
#include <WiFi.h>
#include <time.h>
#include <ArduinoSort.h> // https://github.com/emilv/ArduinoSort/blob/master/examples/SortArray/SortArray.ino
#include <Wire.h>
#include "Adafruit_VL6180X.h"
#include <driver/adc.h>

#define I2C_SDA GPIO_NUM_39
#define I2C_SCL GPIO_NUM_38

#define VLX_SAMPLE_SIZE 100
#define MS_DELAY_PER_SAMPLE 25

Adafruit_VL6180X vlx = Adafruit_VL6180X();
EInkDisplay_VisionMasterE290 display;
ulong lastSyncMillis = 0;

const int FULL_TANK_MM = 5;
const int EMPTY_TANK_MM = 45;
const float TANK_GALLONS = 4.5;
const int MPG_AVG = 40;

char BUFFER[512] = {0};
float STD_DEV = 0;
float AVG_MM = 0;
int PCT_FULL = 0; 
float F_PCT_FULL = 0.0;
float GAL_REMAIN = 0;
RTC_DATA_ATTR int bootCount = 0;

#define BOX_WIDTH 192
#define BOX_HEIGHT 90
#define BOX_TOP 100

#include "common.h"

uint pollData() {
    Serial.println("pollData()");
  
    sample_vlx();
    PCT_FULL = map(AVG_MM, FULL_TANK_MM, EMPTY_TANK_MM, 1, 100); 
    if(PCT_FULL < 0)
    PCT_FULL = 0;

    F_PCT_FULL = (.01 * PCT_FULL);
    float GAL_REMAIN = TANK_GALLONS * F_PCT_FULL;

    uint sleep_time_seconds = 5 * 60;
    if(STD_DEV > 12) 
        sleep_time_seconds = 20;

    memset(BUFFER, 0, sizeof(BUFFER));
    String b = "";
    getLocalTime(&timeinfo);
    strftime(BUFFER, sizeof(BUFFER), "%b %d %Y %H:%M:%S", &timeinfo);
    b += String("Full  :") + String(PCT_FULL) + String("% gal ") + String(GAL_REMAIN, 1) + "\n";
    b += String("Range :") + String(int(MPG_AVG * GAL_REMAIN)) + " miles\n";
    b += String("Fuel Depth :") + String(AVG_MM, 0) + String("mm StdDev ") + String(STD_DEV, 2) + "\n";
    b += String(BUFFER) + "\n";
    //b += String("Batt  :") + String(read_battery(), 2) + "v\n";
    //b += String("Boot  #") + String(bootCount) + " Zz " + String(sleep_time_seconds) + "s\n";
    b.toCharArray(BUFFER, b.length() + 1);
    Serial.println(BUFFER);

    display.clearMemory();  // Start a new drawing
    display.landscape();
    display.setFont( &FreeSans9pt7b );
    display.printCenter(BUFFER);
    display.update();
    return sleep_time_seconds;
}

void setup() {
	Serial.begin(115200);
    delay(200);
    Serial.print("Running ");
    Serial.print(" Version: ");
    Serial.println(__TIMESTAMP__);
    ++bootCount;
    Serial.println("Boot number: " + String(bootCount));
    print_wakeup_reason();
    Wire.setPins(I2C_SDA, I2C_SCL);
    vlx.begin();
    setupWiFi();
    syncTime();
}

void original_setup() {
    display.print("Hello, world!");
    display.update();
    
    delay(4000);
    display.clearMemory();  // Start a new drawing

    display.landscape();
    display.setFont( &FreeSans9pt7b );
    display.printCenter("In the middle.");
    display.update();

    delay(4000);
    display.clearMemory();  // Start a new drawing

    display.setCursor(5, 20);   // Text Cursor - x:5 y:20
    display.println("Here's a normal update.");
    display.update();

    delay(2000);    // Pause to read

    display.fastmodeOn();
    display.println("Here's fastmode,");
    display.update();
    display.println("aka Partial Refresh.");
    display.update();

    delay(2000);    // Pause to read
    
    display.fastmodeOff();
    display.setFont( &FreeSansBold9pt7b );  // Change to bold font
    display.println();
    display.println("Don't use too often!");
    display.update();

    delay(4000);
    display.clear();    // Physically clear the screen

    display.fastmodeOn();
    display.setFont( NULL ); // Default Font
    display.printCenter("How about a dot?", 0, -40);   // 40px above center
    display.update();

    delay(2000);

    display.fastmodeOff();
    display.fillCircle(
        display.centerX(),  // X: center screen
        display.centerY(),  // Y: center screen
        10,                             // Radius: 10px
        BLACK                           // Color: black
        );
    display.update();

    delay(2000);
    
    display.fillCircle(display.centerX(), display.centerY(), 5, WHITE);   // Draw a smaller white circle inside, giving a "ring" shape
    display.update();

    delay(10000);
    display.clear();    // Physically clear the screen, ready for your next masterpiece
}

void loop() {
//checkTimeAndSync();  // Check if 1 hour has passed and sync if necessary
    pollData();
    delay(30 * 1000);
}