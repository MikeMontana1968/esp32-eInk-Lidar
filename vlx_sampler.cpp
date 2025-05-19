#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "string.h"
#include "MyTask.cpp"
#include <Wire.h>
#include <driver/adc.h>
#include <atomic>
#include "Adafruit_VL6180X.h"
#define TAG_VLX "vlx"

#define RING_BUFFER_SIZE 200
typedef struct vlx_state {
  float avg_lidar_mm;
  float std_dev;
  uint  seconds_index;
  uint  measurement_count = RING_BUFFER_SIZE;
  uint  measurements[RING_BUFFER_SIZE];
  uint32_t last_update;
} vlx_state;



class VlxTask : public MyTask {
  private:
    Adafruit_VL6180X  vlx = Adafruit_VL6180X();
    const     int     vlx_sample_reads = 0;
    const     int     ms_delay_per_sample_read = 0;
    float     avg_lidar_mm ;
    float     std_dev;
    uint      seconds_index;
    uint      measurements[RING_BUFFER_SIZE];
    uint32_t last_update = 0;    
public:
    VlxTask(
            gpio_num_t i2c_sda_pin,
            gpio_num_t i2c_slc_pin,
            bool initialize_with_random_data = false, 
            uint _vlx_sample_reads = 100, 
            uint _ms_delay_per_sample_read = 10 
          ): 
              MyTask(TAG_VLX, 4096, 3), 
              vlx_sample_reads(_vlx_sample_reads),  
              ms_delay_per_sample_read(_ms_delay_per_sample_read) 
          {
            Wire.setPins(i2c_sda_pin, i2c_slc_pin);
            vlx.begin();
            if(initialize_with_random_data) {
              for(int i = 0; i < RING_BUFFER_SIZE; i++) {
                measurements[i] = random(0, 255);        
              }
              avg_lidar_mm = random(32, 64);
              std_dev = 1.1;
            } else {
              for(int i = 0; i < RING_BUFFER_SIZE; i++) {
                measurements[i] = 0;        
              }
              avg_lidar_mm = -1;
              std_dev = 0.0;
            }
    }
    void getUpdate(vlx_state* state) {
      ESP_LOGI(TAG_VLX, "Begin");
        SemaphoreHandle_t mutex = xSemaphoreCreateMutex();
        {
            xSemaphoreTake(mutex, portMAX_DELAY); // enter critical section
              state->avg_lidar_mm = avg_lidar_mm;
              state->std_dev = std_dev;
              state->seconds_index = seconds_index;
              state->last_update = last_update;
              for(int i = 0; i < RING_BUFFER_SIZE; i++) {
                state->measurements[i] = measurements[i];        
              }
            xSemaphoreGive(mutex); // exit critical section
        }
        vSemaphoreDelete(mutex);
      ESP_LOGI(TAG_VLX, "End");
    }

    uint getMeasurementCount() {
      return RING_BUFFER_SIZE;
    }
protected:

    float getMean(uint8_t* val, int arrayCount) {
      long total = 0;
      float avg = 0;
      
      for (int i = 0; i < arrayCount; i++) {
        total = total + val[i];
      }
      if(total)
        avg = total/(float)arrayCount;

      //Serial.printf("getMean([], %i)= ", arrayCount); Serial.print("=" + String(avg, 2) + "\n");
      return avg;
    }

    float getStdDev(uint8_t* val, int arrayCount) {
      float avg = getMean(val, arrayCount);
      float variance = 0;
      long total = 0;
      float stdDev = 0;
      for (int i = 0; i < arrayCount; i++) {
        
        total = total + (val[i] - avg) * (val[i] - avg);
      }
      if(total)
        variance = total/(float)arrayCount;
        stdDev = sqrt(variance);
      //Serial.print("getStdDev([], " + String(arrayCount) + ")=" + String(stdDev,2) + "\n");
      return stdDev;
    }

    void run() override {
        while (true) {            
            sample_vlx();
            vTaskDelay(1013 / portTICK_PERIOD_MS);  // Delay for 1 second
        }
    }

    float read_lux() {
      float lux = vlx.readLux(VL6180X_ALS_GAIN_5);
      //Serial.print("Lux: "); Serial.println(lux);
      return lux;
    }

    uint8_t read_vlx() {
      uint8_t range = vlx.readRange();
      uint8_t status = vlx.readRangeStatus();

      if (status == VL6180X_ERROR_NONE) {
        return range;
      }

      // Some error occurred, print it out!
      avg_lidar_mm = -1;
      std_dev = -1;
      if  ((status >= VL6180X_ERROR_SYSERR_1) && (status <= VL6180X_ERROR_SYSERR_5)) {
        Serial.println("System error");
      }
      else if (status == VL6180X_ERROR_ECEFAIL) {
        Serial.println("ECE failure");
      }
      else if (status == VL6180X_ERROR_NOCONVERGE) {
        Serial.println("No convergence");
      }
      else if (status == VL6180X_ERROR_RANGEIGNORE) {
        Serial.println("Ignoring range");
      }
      else if (status == VL6180X_ERROR_SNR) {
        Serial.println("Signal/Noise error");
      }
      else if (status == VL6180X_ERROR_RAWUFLOW) {
        Serial.println("Raw reading underflow");
      }
      else if (status == VL6180X_ERROR_RAWOFLOW) {
        Serial.println("Raw reading overflow");
      }
      else if (status == VL6180X_ERROR_RANGEUFLOW) {
        Serial.println("Range reading underflow");
      }
      else if (status == VL6180X_ERROR_RANGEOFLOW) {
        Serial.println("Range reading overflow");
      }

      return 0;
    }

    float sample_vlx() {
      ulong start = millis();
      uint8_t vlx_samples[vlx_sample_reads+1];
      memset(vlx_samples, sizeof(vlx_samples), 0);
      uint8_t offset = int(vlx_sample_reads * 0.20);
      for(int i =0; i < vlx_sample_reads; i++) {
        vlx_samples[i] = read_vlx();
        delay(ms_delay_per_sample_read);
      }

      std_dev = getStdDev(vlx_samples, vlx_sample_reads);
      avg_lidar_mm = getMean(vlx_samples, vlx_sample_reads);
      last_update = millis();
      measurements[seconds_index] = avg_lidar_mm;
      String b = String("Samples: " + String(vlx_sample_reads) + " Avg: " + String(avg_lidar_mm,1) + " " + String(millis() - start)) + "ms";
      char buf[128] = {0};
      b.toCharArray(buf, b.length() + 1);
      ESP_LOGI(TAG_VLX, "%s", buf);
      return avg_lidar_mm;
    }
};