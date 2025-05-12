extern float STD_DEV;
extern float AVG_MM;
extern Adafruit_VL6180X vlx;
#include <ArduinoSort.h> // https://github.com/emilv/ArduinoSort/blob/master/examples/SortArray/SortArray.ino

float getMean(uint8_t* val, int arrayCount) {
  long total = 0;
  float avg = 0;
  
  for (int i = 0; i < arrayCount; i++) {
    total = total + val[i];
  }
  if(total)
    avg = total/(float)arrayCount;

  Serial.printf("getMean([], %i)= ", arrayCount); Serial.print("=" + String(avg, 2) + "\n");
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
  Serial.printf("sample_vlx(sample_size=%i, delay_ms_per_sample=%i)\n", sample_size, delay_ms_per_sample);
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
  Serial.print("sample_vlx(" + String(sample_size) + ")=" + String(avg,2));
  Serial.printf(" max[%i,%i,%i]", vlx_samples[0], vlx_samples[1], vlx_samples[2]) ;
  Serial.printf(" min[%i,%i,%i]", vlx_samples[sample_size-3], vlx_samples[sample_size-2], vlx_samples[sample_size-1]) ;
  Serial.print(" stdDev " + String(STD_DEV,2)) ;
  Serial.println();
  return avg;
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

    Serial.printf("getUptime(%i) -> %s\n", millis() / 1000UL, result);
}
