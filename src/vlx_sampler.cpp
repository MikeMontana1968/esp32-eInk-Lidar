#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "string.h"
#include "MyTask.cpp"
#include <Wire.h>
#include <driver/adc.h>
#include "Adafruit_VL6180X.h"

#define TAG_VLX "vlx"
#define RING_BUFFER_SIZE 200
#define GPIO_VLX_PIN 18

typedef struct vlx_state {
  uint32_t last_update;
  float avg_lidar_mm;
  float std_dev;
  uint  current_index;
  uint  measurement_count = RING_BUFFER_SIZE;
  uint  measurements[RING_BUFFER_SIZE];
  char  error_message[RING_BUFFER_SIZE];
} vlx_state;

class VlxTask : public MyTask {
  private:
    Adafruit_VL6180X    vlx                       = Adafruit_VL6180X();
    const     int       vlx_sample_reads          = 0;
    const     int       ms_delay_per_sample_read  = 0;
              float     avg_lidar_mm              = 0.0;
              float     std_dev                   = 0.0;
              uint      current_index             = 0;
              uint      measurements[RING_BUFFER_SIZE] = {0};
              uint32_t  last_update               = 0;    
              char      buf[64]                   = {0};
              char      error_message[RING_BUFFER_SIZE] = {0};
public:
    VlxTask(
            gpio_num_t i2c_sda_pin,
            gpio_num_t i2c_slc_pin,
            bool initialize_with_random_data = false, 
            uint _vlx_sample_reads = 20, 
            uint _ms_delay_per_sample_read = 10 
          ): 
              MyTask(TAG_VLX, 4096, 3), 
              vlx_sample_reads(_vlx_sample_reads),  
              ms_delay_per_sample_read(_ms_delay_per_sample_read) 
          {
            // Enable power to peripherals on Vision Master E290 (VEXT = GPIO 18, active HIGH)
            pinMode(GPIO_VLX_PIN, OUTPUT);
            digitalWrite(GPIO_VLX_PIN, HIGH);
            delay(100);  // Allow power to stabilize

            Wire.begin(i2c_sda_pin, i2c_slc_pin);
            if (!vlx.begin(&Wire)) {
                ESP_LOGE(TAG_VLX, "VL6180X not found! Check wiring and I2C address.");
            }
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
            ESP_LOGI(TAG_VLX,"Constructor Complete");
          }

    void getUpdate(vlx_state* state) {
      ESP_LOGV(TAG_VLX, "Begin");
        SemaphoreHandle_t mutex = xSemaphoreCreateMutex();
        {
            xSemaphoreTake(mutex, portMAX_DELAY); // enter critical section
              state->avg_lidar_mm = avg_lidar_mm;
              state->std_dev = std_dev;
              state->current_index = current_index;
              state->last_update = last_update;
              for(int i = 0; i < RING_BUFFER_SIZE; i++) {
                state->measurements[i] = measurements[i];        
              }              
              strcpy(state->error_message, error_message);
            xSemaphoreGive(mutex); // exit critical section
        }
        vSemaphoreDelete(mutex);
      ESP_LOGV(TAG_VLX, "End");
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
            getSampleSet();
            vTaskDelay(1013 / portTICK_PERIOD_MS);  // Delay for 1 second
        }
    }

    float getLuxReading() {
      float lux = vlx.readLux(VL6180X_ALS_GAIN_5);
      //Serial.print("Lux: "); ESP_LOGI(TAG_VLX, lux);
      return lux;
    }

    uint8_t getDistanceReading() {
      uint8_t range = vlx.readRange();
      uint8_t status = vlx.readRangeStatus();      
      if (status == VL6180X_ERROR_NONE) {
        return range;
      }
      avg_lidar_mm = -1;
      std_dev = -1;
      
      if  ((status >= VL6180X_ERROR_SYSERR_1) && (status <= VL6180X_ERROR_SYSERR_5)) {
        strcpy(error_message, "System error");
        return 0;
      }

      if (status == VL6180X_ERROR_ECEFAIL) {
        strcpy(error_message,  "ECE failure");
        return 0;
      }

      if (status == VL6180X_ERROR_NOCONVERGE) {
        strcpy(error_message,  "No convergence");
        return 0;
      }

      if (status == VL6180X_ERROR_RANGEIGNORE) {
        strcpy(error_message,  "Ignoring range");
        return 0;
      }

      if (status == VL6180X_ERROR_SNR) {
        strcpy(error_message,  "Signal/Noise error");
        return 0;
      }

      if (status == VL6180X_ERROR_RAWUFLOW) {
        strcpy(error_message,  "Raw reading underflow");
        return 0;
      }

      if (status == VL6180X_ERROR_RAWOFLOW) {
        strcpy(error_message,  "Raw reading overflow");
        return 0;
      }

      if (status == VL6180X_ERROR_RANGEUFLOW) {
        strcpy(error_message,  "Range reading underflow");
        return 0;
      }

      if (status == VL6180X_ERROR_RANGEOFLOW) {
        strcpy(error_message, "Range reading overflow");
        return 0;
      }

      ESP_LOGE(TAG_VLX, "--> %s", error_message);
      return 0;
    }

    float getSampleSet() {
      ulong start = millis();
      uint8_t vlx_samples[vlx_sample_reads+1];
      memset(error_message, 0, sizeof(error_message));
      memset(vlx_samples, 0, sizeof(vlx_samples));
      for(int i =0; i < vlx_sample_reads; i++) {
        vlx_samples[i] = getDistanceReading();
        delay(ms_delay_per_sample_read);
      }
      std_dev = getStdDev(vlx_samples, vlx_sample_reads);
      avg_lidar_mm = getMean(vlx_samples, vlx_sample_reads);
      last_update = millis();
      //current_index = (last_update / 1000) % RING_BUFFER_SIZE;
      current_index++;
      if (current_index >= RING_BUFFER_SIZE)
        current_index = 0;

      measurements[current_index] = avg_lidar_mm;      
      String b = String("# " + String(current_index) + " Avg: " + String(avg_lidar_mm,1) + "mm " + String(millis() - start)) + "ms";
      b.toCharArray(buf, b.length() + 1);
      ESP_LOGI(TAG_VLX, "%s %s", buf, error_message);
      return avg_lidar_mm;
    }
};