#pragma once
#include <heltec-eink-modules.h>
#include "Fonts/FreeSans9pt7b.h"
#include "Fonts/FreeSansBold9pt7b.h"
#include "Fonts/FreeSansBold12pt7b.h"
#include "Fonts/FreeMono9pt7b.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "string.h"
#include "task.h"

#include <Wire.h>
#include <driver/adc.h>

#include <Adafruit_GFX.h>    // Core graphics library

extern volatile float AVG_MM;
extern uint    measurements[];
extern const uint MAX_MEASUREMENTS;
extern volatile uint seconds_index;

class EinkDisplayTask : public Task {
public:
    EinkDisplayTask() : Task("EinkDisplayTask", 2048, 3), nBlink(0) {
        Wire.setPins(I2C_SDA, I2C_SCL);
    }

    void getUptime(char *result) {
        unsigned long seconds = millis() / 1000UL;
        //seconds += 3600;
        int days = seconds / 86400; // 86400 seconds in a day
        seconds %= 86400;
        int hours = seconds / 3600; // 3600 seconds in an hour
        seconds %= 3600;
        int minutes = seconds / 60;
        seconds %= 60;

        if (days >= 1) {
            sprintf(result, "%dd %d%h", days, hours);
        } else if (hours >= 1) {
            sprintf(result, "%dh %dm", hours, minutes);
        } else {
            sprintf(result, "%dm %ds", minutes, seconds);
        }

       // Serial.printf("getUptime(%i) -> %s\n", millis() / 1000UL, result);
    }
    void read_globals() {
        // TODO Wrap this in thread-safety mutex
        localAvg = AVG_MM;
        localStdDev = STD_DEV;
        localSecIndex = seconds_index;
    }

    void updateDisplay() {
        Serial.printf("updateDisplay(");
        read_globals();
        if(localAvg < 0) {
            noI2CReadings();
            return;
        }
        percentFull = map(localAvg, FULL_TANK_MM, EMPTY_TANK_MM, 1, 100); 
        //PCT_FULL = random(0, 100);
        if(percentFull < 0)
            percentFull = 0;
        if(localAvg < FULL_TANK_MM)
            percentFull = 100;

        // https://github.com/todd-herbert/heltec-eink-modules/blob/main/docs/README.md#drawline
        // https://learn.adafruit.com/adafruit-gfx-graphics-library/graphics-primitives
        display.clearMemory();  // Start a new drawing
        display.landscape();
        display.setTextColor(BLACK);
        display.setFont( &FreeSans9pt7b );
            drawTankFull();
            drawTankHistory();
        display.update();
        Serial.println(")");
        return;
    }

    void drawTankFull() {
        // ------------------------ Percent Full Bar graph -----------------------------
        char szTemp[80] = {0};
        GAL_REMAIN = TANK_GALLONS * (.01 * percentFull);
        b = String(GAL_REMAIN, 1) + " gal";
        if(GAL_REMAIN < 1) {
            b = " GET GAS!";
        }
        b.toCharArray(szTemp, b.length() + 1);
        display.setFont( &FreeSans9pt7b );
        display.drawRect(0,0, bargraph_width, bargraph_height, WHITE);
        display.drawRoundRect(0,0, bargraph_width, bargraph_height, radius, BLACK);

        uint16_t w = map(percentFull, 0, 100, offset, bargraph_width - offset);
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
            display.setCursor(bx + 4, by + text_height * 1.5);
            display.setTextColor(WHITE);
        } else {
            // the pct-full graph is really low, so the text is too wide. Position the cursor just after the graph and print in Black
            display.setCursor(bw - text_width - offset, by + text_height * 1.5);
            display.setTextColor(BLACK);
        }
        display.print(szTemp); // eg "4.5 gal" or "GET GAS!"
        Serial.printf("%s ", szTemp);

        // ---- Print the range to the right of the PctFull graph --------
        display.setFont( &FreeSansBold12pt7b );
        memset(buf, 0, sizeof(buf));
        b = String(int(MPG_AVG * GAL_REMAIN)) + "mi";
        b.toCharArray(buf, b.length() + 1);
        text_width = display.getTextWidth(buf);
        text_height = display.getTextHeight(buf);
        display.setTextColor(BLACK);
        display.setCursor(DISPLAY_WIDTH - text_width - 6, text_height *1.5);
        display.print(buf); // eg "127 mi"
        Serial.print(buf);
        // ---- Print the LIDAR... Ping Depth ----    
        display.setFont( &FreeSans9pt7b );
        memset(szTemp, sizeof(szTemp), 0);
        getUptime(szTemp); 
        b  =  "Lidar: " + String(localAvg, 0) + String("mm (d=") + String(localStdDev, 1) + ") Up " + szTemp;
        memset(buf, 0, sizeof(buf));
        b.toCharArray(buf, b.length() + 1);
        text_width = display.getTextWidth(buf);
        text_height = display.getTextHeight(buf);
        display.setCursor(0, bargraph_height + 20 );
        display.print(buf);        
        Serial.printf(" %s\n",buf);        
    }

    void drawTankHistory() {        
        uint graph_top = DISPLAY_HEIGHT * 0.66;

        // Display a history bar graph for all measurements
        for(uint x = 0; x < MAX_MEASUREMENTS; x++) {
            int y = map(measurements[x], FULL_TANK_MM, EMPTY_TANK_MM, DISPLAY_HEIGHT, graph_top); // map(value, fromLow, fromHigh, toLow, toHigh)
            display.drawLine(x, DISPLAY_HEIGHT, x, y, BLACK);
        }
        
        // Add a text label centered on the bottom edge 
        b = "Tank";
        b.toCharArray(buf, b.length() + 1);
        text_width = display.getTextWidth(buf);
        text_height = display.getTextHeight(buf);
        int half_width = (text_width /2);
        int16_t  label_x = (DISPLAY_WIDTH/2) - half_width ;
        int16_t  label_y = DISPLAY_HEIGHT - text_height;
        display.setTextColor(BLACK);

        // Erase the background for the TANK label
        display.fillRect(
            label_x - offset, label_y - offset, 
            text_width + offset*2, text_height + offset, 
            WHITE
        );
        // Write the label eg "Tank"
        display.setCursor(label_x, label_y + text_height - offset*3);
        display.print(buf); 
        
        // Erase two vertival bars to the right of the current 'seconds'
        display.fillRect(
            localSecIndex+1, graph_top,
            localSecIndex+3, DISPLAY_HEIGHT - graph_top, 
            WHITE
        );
        //Draw an arrow atop the history
        for(int lazy = 8; lazy > 0; lazy--) {
            display.drawLine(
                localSecIndex-lazy, graph_top-lazy,
                localSecIndex+lazy, graph_top-lazy,
                BLACK
            );
        }
        display.drawLine(
            localSecIndex-2, graph_top-2,
            localSecIndex+2, graph_top-2, 
            BLACK
        );

        // draw border around the history bar-graph
        display.drawRect(
            0, graph_top, 
            DISPLAY_WIDTH, DISPLAY_HEIGHT - graph_top, 
            BLACK
        );
    }

    void noI2CReadings() {
        display.clearMemory();  // Start a new drawing
        display.landscape();
        display.setTextColor(BLACK);
        display.setFont( &FreeMono9pt7b );        
        display.printCenter("Not getting Sensor Data!");
        display.update();
        delay(5000);
    }

    void splashScreen() {
        display.clearMemory();  // Start a new drawing
        display.landscape();
        display.setTextColor(BLACK);
        display.setFont( &FreeMono9pt7b );
        memset(buf, 0, sizeof(buf));
        char szBuf[10] = {0};

        dtostrf( TANK_GALLONS, 4, 2, szBuf );
        display.setCursor(0, 20);
        
        int x  = sprintf(buf    , "%s\n", TITLE);
            x += sprintf(buf + x, " Ver: %s\n", __DATE__);
            x += sprintf(buf + x, "Tank: %s gal, %dmpg\n", szBuf, MPG_AVG);
            x += sprintf(buf + x, "Full: %i mm, Empty %i mm\n",  FULL_TANK_MM, EMPTY_TANK_MM);
            x += sprintf(buf + x, " VLX: %i samples @ %ims\n",  VLX_SAMPLE_SIZE, MS_DELAY_PER_SAMPLE);        
            x += sprintf(buf + x, "Screen Updates: %is\n", UPDATE_REFRESH_SEC);
        
        display.print(buf);
        display.update();
        delay(4000);
    }

protected:
    void run() override {
        splashScreen();
        while (true) {
            updateDisplay();
            vTaskDelay(10 * 1000 / portTICK_PERIOD_MS);  // Delay for 1 second
        }
    }

private:
    uint    nBlink;    
    int     percentFull = 0;   // updateDisplay()
    float   GAL_REMAIN = 0;   // updateDisplay
    float   localAvg = 0.0;
    float   localStdDev = 0.0;
    char    buf[256] = {0};
    uint    localSecIndex = 0;
    uint16_t text_width = 0;
    uint16_t text_height = 0;
    uint16_t radius = 3;
    uint16_t bargraph_height = 40;
    uint16_t bargraph_width = 225;
    uint16_t offset = 1;
    String b = "";
    EInkDisplay_VisionMasterE290 display;
};