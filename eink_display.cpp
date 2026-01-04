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
#include "MyTask.cpp"
#include "vlx_sampler.cpp"
#include <Wire.h>
#include <driver/adc.h>

#include <Adafruit_GFX.h>    // Core graphics library

#define TAG_EINK "eink"

// Tank constants for polynomial fuel calculation
const float TANK_MAX_HEIGHT_MM = 152.0f;
const float TANK_CAPACITY_GAL = 4.5f;
const float TANK_CAPACITY_L = TANK_CAPACITY_GAL * 3.78541f;  // ~17.03 L

// 5th-degree polynomial coefficients for volume from normalized height
// Derived from Honda Goldwing tank STL voxel analysis (R² = 0.999)
float getVolumeFromHeight(float height_mm) {
    float h = height_mm / TANK_MAX_HEIGHT_MM;  // normalize 0-1
    float v = -12.240043f * pow(h, 5)
            + 37.229361f * pow(h, 4)
            - 41.972364f * pow(h, 3)
            + 19.803335f * pow(h, 2)
            - 1.863900f * h
            + 0.035199f;
    return constrain(v * TANK_CAPACITY_L, 0, TANK_CAPACITY_L);
}

// Get gallons remaining from fuel height (mm)
float getGallonsFromHeight(float height_mm) {
    return getVolumeFromHeight(height_mm) / 3.78541f;
}

// Get fill percentage (0-100) from fuel height (mm)
float getFillPercent(float height_mm) {
    return (getVolumeFromHeight(height_mm) / TANK_CAPACITY_L) * 100.0f;
}

class EinkDisplayTask : public MyTask {
    private:
    uint    nBlink;    
    int     percentFull = 0;   // updateDisplay()
    float   galRemain = 0;   // updateDisplay
    char    buf[256] = {0};
    uint16_t text_width = 0;
    uint16_t text_height = 0;
    uint16_t radius = 3;
    uint16_t bargraph_height = 40;
    uint16_t bargraph_width = 225;
    uint16_t offset = 1;
    String b = "";
    VlxTask* vlxTask;
    EInkDisplay_VisionMasterE290 display;

    const int   mmFullTank;
    const int   mmEmptyTank;
    const float galCapacity;
    const int   mpgAvg;
    const int   secRefresh;
    char sFuelRemain[80] = {0};
    char sRangeEst[10] = {0};
    char sLidarUptime[80] = {0};
    vlx_state data= {0};
public:
    EinkDisplayTask(                
                gpio_num_t i2c_sda_pin,
                gpio_num_t i2c_slc_pin, 
                VlxTask* _VlxTask, 
                uint _mmFullTank = 10, 
                uint _mmEmptyTank = 152, 
                float _galCapacity = 4.5, 
                int _mpgAvg = 40, 
                int _secRefresh = 30
            ) : 
            MyTask(TAG_EINK, 2048, 3), 
            nBlink(0), 
            vlxTask(_VlxTask), 
            mmFullTank(_mmFullTank),
            mmEmptyTank(_mmEmptyTank),
            mpgAvg(_mpgAvg),
            galCapacity(_galCapacity),
            secRefresh(_secRefresh) 
        {
        Wire.setPins(i2c_sda_pin, i2c_slc_pin);
        ESP_LOGI(TAG_EINK, "Full %dmm Empty %dmm", _mmFullTank, _mmEmptyTank);
        ESP_LOGI(TAG_EINK,"Constructor Complete");
    }

    void getUptime(char *result) {
        unsigned long seconds = millis() / 1000UL;
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

    void read_lidar_values() {
       vlxTask->getUpdate(&data);
        // SemaphoreHandle_t mutex = xSemaphoreCreateMutex();
        // {
        //     xSemaphoreTake(mutex, portMAX_DELAY); // enter critical section
        //      ESP_LOGI(TAG_EINK, "Begin");
        //      vlxTask->getUpdate(&data);
        //     xSemaphoreGive(mutex); // exit critical section
        // }
        // vSemaphoreDelete(mutex);
    }

    void updateDisplay() {
        char szTemp[32] = {0};
        read_lidar_values();
        if(data.avg_lidar_mm < 0) {
            noI2CReadings();
            return;
        }

        // Convert LIDAR distance to fuel height (LIDAR measures distance from sensor to fuel surface)
        float fuel_height_mm = mmEmptyTank - data.avg_lidar_mm;
        if(fuel_height_mm < 0) fuel_height_mm = 0;
        if(fuel_height_mm > TANK_MAX_HEIGHT_MM) fuel_height_mm = TANK_MAX_HEIGHT_MM;

        // Use polynomial to calculate fuel volume and percentage
        galRemain = getGallonsFromHeight(fuel_height_mm);
        percentFull = (int)getFillPercent(fuel_height_mm);
        if(percentFull > 100) percentFull = 100;
        if(percentFull < 0) percentFull = 0;

        sFuelRemain[80] = {0};
        b = String(galRemain, 1) + " gal";
        if(galRemain < 1) {
            b = " GET GAS!";
        }
        b.toCharArray(sFuelRemain, b.length() + 1);

        memset(sRangeEst, 0, sizeof(sRangeEst));
        b = String(int(mpgAvg * galRemain)) + "mi";
        b.toCharArray(sRangeEst, b.length() + 1);

        memset(szTemp, sizeof(szTemp), 0);
        getUptime(szTemp); 

        if(strlen(data.error_message)) {
            b = String(data.error_message) + " " + szTemp;
        } else {
            b  =  "Lidar: " + String(data.avg_lidar_mm, 0) + String("mm (d=") + String(data.std_dev, 1) + ") Up " + szTemp;
        }
        
        memset(sLidarUptime, sizeof(sLidarUptime), 0);
        b.toCharArray(sLidarUptime, b.length() + 1);

        ESP_LOGI(TAG_EINK, "%s, %s, %s", sFuelRemain, sRangeEst, sLidarUptime);
        // https://github.com/todd-herbert/heltec-eink-modules/blob/main/docs/README.md#drawline
        // https://learn.adafruit.com/adafruit-gfx-graphics-library/graphics-primitives
        display.clearMemory();  // Start a new drawing
        display.landscape();
        display.setTextColor(BLACK);
        display.setFont( &FreeSans9pt7b );
            drawTankFull();
            drawTankHistory();
        display.update();
        return;
    }

    void drawTankFull() {
        // ------------------------ Percent Full Bar graph -----------------------------
    
        display.setFont( &FreeSans9pt7b );
        display.drawRect(0,0, bargraph_width, bargraph_height, WHITE);
        display.drawRoundRect(0,0, bargraph_width, bargraph_height, radius, BLACK);

        uint16_t w = map(percentFull, 0, 100, offset, bargraph_width - offset);
        uint16_t bx, by, bh, bw = 0;
        text_width = display.getTextWidth(sFuelRemain);
        text_height = display.getTextHeight(sFuelRemain);
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
            display.setCursor(bw + offset, by + text_height * 2);
            display.setTextColor(BLACK);
        }
        display.print(sFuelRemain); // eg "4.5 gal" or "GET GAS!"

        // ---- Print the range to the right of the PctFull graph --------
        display.setFont( &FreeSansBold12pt7b );

        text_width = display.getTextWidth(sRangeEst);
        text_height = display.getTextHeight(sRangeEst);
        display.setTextColor(BLACK);
        display.setCursor(DISPLAY_WIDTH - text_width - 6, text_height *1.5);
        display.print(sRangeEst); // eg "127 mi"

        // ---- Print the LIDAR... Ping Depth ----    
        display.setFont( &FreeSans9pt7b );

        text_width = display.getTextWidth(sLidarUptime);
        text_height = display.getTextHeight(sLidarUptime);
        display.setCursor(0, bargraph_height + 20 );
        display.print(sLidarUptime);        
    }

    void drawTankHistory() {        
        uint graph_top = DISPLAY_HEIGHT * 0.66;

        // Display a history bar graph for all measurements
        for(uint x = 0; x < data.measurement_count; x++) {
            int y = map(data.measurements[x], mmFullTank, mmEmptyTank, DISPLAY_HEIGHT, graph_top); // map(value, fromLow, fromHigh, toLow, toHigh)
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
        // display.fillRect(
        //     data.current_index+1, graph_top,
        //     data.current_index+3, DISPLAY_HEIGHT - graph_top, 
        //     WHITE
        // );
        //Draw an arrow atop the history
        for(int lazy = 8; lazy > 0; lazy--) {
            display.drawLine(
                data.current_index-lazy, graph_top-lazy,
                data.current_index+lazy, graph_top-lazy,
                BLACK
            );
        }
        display.drawLine(
            data.current_index-2, graph_top-2,
            data.current_index+2, graph_top-2, 
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
        if(strlen(data.error_message))
            display.printCenter(data.error_message);
        else
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

        dtostrf( galCapacity, 4, 2, szBuf );
        display.setCursor(0, 20);
        
        int x  = sprintf(buf    , "%s\n", "Mike Montana");
            x += sprintf(buf + x, " Ver: %s\n", __DATE__);
            x += sprintf(buf + x, "Tank: %s gal, %dmpg\n", szBuf, mpgAvg);
            x += sprintf(buf + x, "Full: %i mm, Empty %i mm\n",  mmFullTank, mmEmptyTank);
            x += sprintf(buf + x, "Screen Updates: %is\n", secRefresh);
        
        display.print(buf);
        display.update();
        delay(4000);
    }

protected:
    void run() override {
        splashScreen();
        while (true) {
            updateDisplay();
            vTaskDelay(secRefresh * 1000 / portTICK_PERIOD_MS);
        }
    }
};