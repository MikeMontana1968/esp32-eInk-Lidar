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

#define I2C_SDA GPIO_NUM_39
#define I2C_SCL GPIO_NUM_38

class VlxTask : public MyTask {
  private:
    Adafruit_VL6180X  vlx = Adafruit_VL6180X();
    const     int     vlx_sample_reads = 0;
    const     int     ms_delay_per_sample_read = 0;
    std::atomic<float>   avg_lidar_mm ;
    std::atomic<float>   std_dev;
    std::atomic<uint>    seconds_index;
    std::atomic<uint>    measurements[RING_BUFFER_SIZE];
    uint32_t last_update = 0;
public:
    VlxTask(
      bool initialize_with_random_data = false, 
      uint _vlx_sample_reads = 100, 
      uint _ms_delay_per_sample_read = 10 ) : 
              MyTask(TAG_VLX, 4096, 3), 
              vlx_sample_reads(_vlx_sample_reads),  
              ms_delay_per_sample_read(_ms_delay_per_sample_read) 
          {
            Wire.setPins(I2C_SDA, I2C_SCL);
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
        state->avg_lidar_mm = avg_lidar_mm;
        state->std_dev = std_dev;
        state->seconds_index = seconds_index;
        state->last_update = last_update;
        for(int i = 0; i < RING_BUFFER_SIZE; i++) {
          state->measurements[i] = measurements[i];        
        }
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

    void mutex_update(float avg, float std) {
        ESP_LOGI(TAG_VLX, "Begin");
        SemaphoreHandle_t mutex = xSemaphoreCreateMutex();
        {
            xSemaphoreTake(mutex, portMAX_DELAY); // enter critical section
              avg_lidar_mm = avg;
              seconds_index = int(millis()/60000) % RING_BUFFER_SIZE;
              std_dev = std;
              last_update = millis();
              measurements[seconds_index] = avg;
            xSemaphoreGive(mutex); // exit critical section
        }
        vSemaphoreDelete(mutex);
      ESP_LOGI(TAG_VLX, "End");
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
      mutex_update(-1, -1);
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

      float std = getStdDev(vlx_samples, vlx_sample_reads);
      float avg = getMean(vlx_samples, vlx_sample_reads);
      mutex_update(avg, std);
      String b = String("Samples: " + String(vlx_sample_reads) + " Avg: " + String(avg,1) + " " + String(millis() - start));
      char buf[128] = {0};
      b.toCharArray(buf, b.length() + 1);
      ESP_LOGI(TAG_VLX, "%s", buf);
      return avg;
    }
};