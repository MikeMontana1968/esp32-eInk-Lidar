#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "string.h"
#include "task.h"
#include "Adafruit_VL6180X.h"
#include <Wire.h>
#include <driver/adc.h>
#include <ArduinoSort.h> // https://github.com/emilv/ArduinoSort/blob/master/examples/SortArray/SortArray.ino

extern uint    measurements[];
extern volatile float AVG_MM;
extern volatile float STD_DEV;
extern const uint MAX_MEASUREMENTS;
extern volatile uint seconds_index;

const int   VLX_SAMPLE_SIZE = 10;
const int   MS_DELAY_PER_SAMPLE = 100;

class VlxTask : public Task {
public:
    VlxTask(bool initialize_with_random_data = false) : Task("vlxTask", 2048, 3) {
      Wire.setPins(I2C_SDA, I2C_SCL);
      vlx.begin();
      if(initialize_with_random_data) {
        for(int i = 0; i < MAX_MEASUREMENTS; i++) {
          measurements[i] = random(0, 255);        
        }
        AVG_MM = random(32, 64);
        STD_DEV = 1.1;
      }
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
            set_global_avg_mm(sample_vlx());
            vTaskDelay(1013 / portTICK_PERIOD_MS);  // Delay for 1 second
        }
    }

    void set_global_avg_mm(float avg) {
      AVG_MM = avg;
      seconds_index = int(millis()/60000) % MAX_MEASUREMENTS;
      measurements[seconds_index] = AVG_MM;
    }

    float read_lux() {
      float lux = vlx.readLux(VL6180X_ALS_GAIN_5);
      Serial.print("Lux: "); Serial.println(lux);
      return lux;
    }

    uint8_t read_vlx() {
      uint8_t range = vlx.readRange();
      uint8_t status = vlx.readRangeStatus();

      if (status == VL6180X_ERROR_NONE) {
        return range;
      }

      // Some error occurred, print it out!
      
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

    float sample_vlx(uint sample_size = VLX_SAMPLE_SIZE, uint delay_ms_per_sample = MS_DELAY_PER_SAMPLE) {
      //Serial.printf("sample_vlx(sample_size=%i, delay_ms_per_sample=%i)\n", sample_size, delay_ms_per_sample);
      ulong start = millis();
      uint8_t vlx_samples[sample_size+1];
      memset(vlx_samples, sizeof(vlx_samples), 0);
      uint8_t offset = int(sample_size * 0.20);
      for(int i =0; i < sample_size; i++) {
        vlx_samples[i] = read_vlx();
        delay(delay_ms_per_sample);
      }
      sortArrayReverse(vlx_samples, sample_size);

      uint sum = 0;  
      uint included_sample_count = 0;
      for(uint i = offset; i < (sample_size - offset); i++) {
        sum += vlx_samples[i];
        included_sample_count++;
      }
      float avg = sum / included_sample_count;
      STD_DEV = getStdDev(vlx_samples, sample_size);
      Serial.println("sample_vlx(" + String(sample_size) + ")=" + String(avg,2));
      // Serial.printf(" max[%i,%i,%i]", vlx_samples[0], vlx_samples[1], vlx_samples[2]) ;
      // Serial.printf(" min[%i,%i,%i]", vlx_samples[sample_size-3], vlx_samples[sample_size-2], vlx_samples[sample_size-1]) ;
      // Serial.print(" stdDev " + String(STD_DEV,2)) ;
      // Serial.println();
      return avg;
    }

private:
    
    Adafruit_VL6180X vlx = Adafruit_VL6180X();
};