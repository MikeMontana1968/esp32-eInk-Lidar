/*
Set Board Type to: Vision Master E290
https://github.com/todd-herbert/heltec-eink-modules/blob/main/docs/WirelessPaper/wireless_paper.md

*/

#include <heltec-eink-modules.h>
#include "Fonts/FreeSans9pt7b.h"
#include "Fonts/FreeSansBold9pt7b.h"
#include "Fonts/FreeSansBold12pt7b.h"
#include <WiFi.h>
#include <time.h>
#include <ArduinoSort.h> // https://github.com/emilv/ArduinoSort/blob/master/examples/SortArray/SortArray.ino
#include <Wire.h>
#include "Adafruit_VL6180X.h"
#include <driver/adc.h>
#include <Adafruit_GFX.h>    // Core graphics library

#define I2C_SDA GPIO_NUM_39
#define I2C_SCL GPIO_NUM_38

#define VLX_SAMPLE_SIZE 100
#define MS_DELAY_PER_SAMPLE 25

Adafruit_VL6180X vlx = Adafruit_VL6180X();

EInkDisplay_VisionMasterE290 display;
ulong lastSyncMillis = 0;

const int FULL_TANK_MM = 15;
const int EMPTY_TANK_MM = (7 * 25.4);
const float TANK_GALLONS = 4.5;
const int MPG_AVG = 40;
const int UPDATE_REFRESH_SEC = 60;

char BUFFER[512] = {0};
float STD_DEV = 0; // set in sample_vlx()
float AVG_MM = 0; // set in updateDisplay()
int PCT_FULL = 0;   // updateDisplay()
float GAL_REMAIN = 0;   // updateDisplay
RTC_DATA_ATTR int bootCount = 0;

#include "common.h"
int iteration = 0;
const int MAX_MEASUREMENTS = DISPLAY_WIDTH;
uint measurements[MAX_MEASUREMENTS] = {0};

bool startupDisplay() {
    display.clearMemory();  // Start a new drawing
    display.landscape();
    display.setTextColor(BLACK);
    display.setFont( &FreeSans9pt7b );
    memset(BUFFER, 0, sizeof(BUFFER));
    char szBuf[10] = {0};

    dtostrf( TANK_GALLONS, 4, 2, szBuf );
    display.setCursor(0, 20);
    
    int x  = sprintf(BUFFER    , "Ver:'%s'\n", __TIMESTAMP__);
        x += sprintf(BUFFER + x, "Tank Size: %s gal, %dmpg\n", szBuf, MPG_AVG);
        x += sprintf(BUFFER + x, "Full > %i mm, Empty < %i mm\n",  FULL_TANK_MM, EMPTY_TANK_MM);
        x += sprintf(BUFFER + x, "VLX: %i samples @ %ims\n",  VLX_SAMPLE_SIZE, MS_DELAY_PER_SAMPLE);        
        x += sprintf(BUFFER + x, "Screen Updates: %is\n", UPDATE_REFRESH_SEC);
    
    display.print(BUFFER);
    display.update();
    delay(4000);
    return true;
}

uint updateDisplay() {
    uint16_t text_width = 0;
    uint16_t text_height = 0;
    uint16_t radius = 3;
    uint16_t bargraph_height = 40;
    uint16_t bargraph_width = 225;
    uint16_t offset = 1;
    String b = "";
    Serial.println("updateDisplay()");
    AVG_MM = sample_vlx();
    PCT_FULL = map(AVG_MM, FULL_TANK_MM, EMPTY_TANK_MM, 1, 100); 
    //PCT_FULL = random(0, 100);
    if(PCT_FULL < 0)
        PCT_FULL = 0;
    if(AVG_MM < FULL_TANK_MM)
        PCT_FULL = 100;
    
    float GAL_REMAIN = TANK_GALLONS * (.01 * PCT_FULL);
    
    iteration = int(millis()/60000) % MAX_MEASUREMENTS;
    measurements[iteration] = AVG_MM;

    // https://github.com/todd-herbert/heltec-eink-modules/blob/main/docs/README.md#drawline
    // https://learn.adafruit.com/adafruit-gfx-graphics-library/graphics-primitives
    display.clearMemory();  // Start a new drawing
    display.landscape();
    display.setTextColor(BLACK);
    display.setFont( &FreeSans9pt7b );

// ------------------------ Percent Full Bar graph -----------------------------
    char szTemp[80] = {0};
    b = String(GAL_REMAIN, 1) + " gal";
    if(GAL_REMAIN < 1) {
        b = " GET GAS!";
    }
    b.toCharArray(szTemp, b.length() + 1);
    display.setFont( &FreeSans9pt7b );

    display.drawRect(0,0, bargraph_width, bargraph_height, WHITE);
    display.drawRoundRect(0,0, bargraph_width, bargraph_height, radius, BLACK);

    uint16_t w = map(PCT_FULL, 0, 100, offset, bargraph_width - offset);
    uint16_t bx, by, bh, bw = 0;
    text_width = display.getTextWidth(szTemp);
    text_height = display.getTextHeight(szTemp);
    bx = offset;
    by = offset;
    bh = bargraph_height - offset-offset;
    bw = w;

    display.fillRoundRect(bx, by, bw, bh, radius, BLACK);

    if(bw > text_width) {
        // the pct-full graph would fit the text, so left align the text in white
        display.setCursor(bx, by + text_height * 1.5);
        display.setTextColor(WHITE);
    } else {
        // the pct-full graph is really low, so the text is too wide. Position the cursor just after the graph and print in Black
        display.setCursor(bw - text_width - offset, by + text_height * 1.5);
        display.setTextColor(BLACK);
    }
    display.print(szTemp); // eg "4.5 gal" or "GET GAS!"
    
    // ---- Print the range to the right of the PctFull graph --------
    display.setFont( &FreeSansBold12pt7b );
    memset(BUFFER, 0, sizeof(BUFFER));
    b = String(int(MPG_AVG * GAL_REMAIN)) + "mi";
    b.toCharArray(BUFFER, b.length() + 1);
    text_width = display.getTextWidth(BUFFER);
    text_height = display.getTextHeight(BUFFER);
    display.setTextColor(BLACK);
    display.setCursor(DISPLAY_WIDTH - text_width - 10, text_height *1.5);
    display.print(BUFFER); // eg "127 mi"

    // ---- Print the LIDAR... Ping Depth ----    
    display.setFont( &FreeSans9pt7b );
    memset(szTemp, sizeof(szTemp), 0);
    getUptime(szTemp); 
    b  =  "Lidar: " + String(AVG_MM, 0) + String("mm d=") + String(STD_DEV, 1) + " On " + szTemp;

    memset(BUFFER, 0, sizeof(BUFFER));
    b.toCharArray(BUFFER, b.length() + 1);
    text_width = display.getTextWidth(BUFFER);
    text_height = display.getTextHeight(BUFFER);
    display.setCursor(0, bargraph_height + 20 );
    display.print(BUFFER);

    // --------------------- Line Graph ----------------------
    uint from_y = DISPLAY_HEIGHT;
    uint to_y = DISPLAY_HEIGHT * 0.66;
    // display.drawRect(
    //     0, to_y, 
    //     DISPLAY_WIDTH, DISPLAY_HEIGHT-2, 
    //     BLACK
    // );
    for(uint x = 0; x < MAX_MEASUREMENTS; x++) {
        // map(value, fromLow, fromHigh, toLow, toHigh)
        int y = map(measurements[x], FULL_TANK_MM, EMPTY_TANK_MM, DISPLAY_HEIGHT, to_y);
        display.drawLine(x, from_y, x, y, BLACK);
    }
    b = "Tank";
    b.toCharArray(BUFFER, b.length() + 1);
    text_width = display.getTextWidth(BUFFER);
    text_height = display.getTextHeight(BUFFER);
    int half_width = (text_width /2);
    int16_t  label_x = (DISPLAY_WIDTH/2) - half_width ;
    int16_t  label_y = DISPLAY_HEIGHT - text_height;
    display.setTextColor(BLACK);
    display.fillRect(
        label_x - offset, label_y + offset, 
        text_width + offset*2, text_height + offset, 
        WHITE
    );

    display.setCursor(label_x, label_y + text_height - offset);
    display.print(BUFFER); //eg "Tank"


    display.update();
    return 0;
}

void setup() {
	Serial.begin(115200);
    
    Serial.print("Running ");
    Serial.print(" Version: ");
    Serial.println(__TIMESTAMP__);
    ++bootCount;
    Serial.println("Boot number: " + String(bootCount));
    print_wakeup_reason();

    startupDisplay();
    
    Wire.setPins(I2C_SDA, I2C_SCL);
    vlx.begin();

    // for(int i = 0; i < MAX_MEASUREMENTS; i++) {
    //     measurements[i] = random(0, 255);
    // }
}

void loop() {
//checkTimeAndSync();  // Check if 1 hour has passed and sync if necessary

    updateDisplay();
    delay(UPDATE_REFRESH_SEC * 1000);
}