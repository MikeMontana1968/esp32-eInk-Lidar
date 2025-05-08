extern float STD_DEV;
extern float AVG_MM;
extern Adafruit_VL6180X vlx;

#include ".credentials.h"

struct tm timeinfo;

// NTP server details
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;  // Offset for GMT in seconds
const int daylightOffset_sec = -4 * 60 * 60;  // Daylight savings time in seconds

// void drawBorder(uint x, uint y, uint w, uint h, uint thickness = 4, uint color = BLACK) {
//   for(uint i = thickness; i > 0; i--){
//     display.drawRect(x, y, w, h, color);
//     x++; y++; w-=2; h-=2;
//   }
// }
// void user_interactive_config() {
//     paint("CONFIG");
//     // rtc.setTime(input_long("Enter current unix epoch:"));
//     // rtc.offset = -4 * 60;
//     FULL_TANK_MM = input_long("Enter Tank depth in mm (eg 127)");
//     EMPTY_TANK_MM = input_long("Enter mm to full-line of tank in mm (eg 10)");
//     TANK_GALLONS = input_float("Enter tank capacity in gallons (eg 3)");
// }

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

long input_long(String prompt) {
    Serial.println(prompt);
    while (Serial.available()) {
      Serial.read();
    }
    while(Serial.available() == 0){}
    return Serial.parseInt();
}

long input_float(String prompt) {
    Serial.println(prompt);
    while (Serial.available()) {
      Serial.read();
    }
    while(Serial.available() == 0){}
    return Serial.parseFloat();
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

esp_sleep_wakeup_cause_t print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : 
      Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); 
      break;
  }
  return wakeup_reason;
}

void setupWiFi() {
    WiFi.begin(ssid, password);  // Connect to WiFi
    uint attempts = 20;
    Serial.printf("setupWiFi(ssid='%s', password='%s') upto %i connect-attempts\n", ssid, password, attempts);    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        attempts--;
        if(attempts < 1) {
          Serial.println("\nWIFI NOT REACHABLE");
          return;
        }
    }
    Serial.println("WiFi connected.");
}

void syncTime() {
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);  // Configure time with NTP server
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println("\nESP32 Time synchronized with NTP server.");
  Serial.print("Current time: ");
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");


    // disconnect WiFi
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);

  lastSyncMillis = millis();  // Record the last sync time in milliseconds
}

void checkTimeAndSync() {
  // Check if 1 hour has passed since the last sync (1 hour = 3600000 milliseconds)
  if (millis() - lastSyncMillis >= 3600000) {
    Serial.println("Synchronizing time with NTP...");
    syncTime();
  }
}

void getUptime(char *result) {
    unsigned long seconds = millis() / 1000UL;
     
    int days = seconds / 86400; // 86400 seconds in a day
    seconds %= 86400;
    int hours = seconds / 3600; // 3600 seconds in an hour
    seconds %= 3600;
    int minutes = seconds / 60;
    seconds %= 60;

    if (days >= 2) {
        sprintf(result, "%d%s %d%s", 
                days, (days > 0 ? "d" : ""), 
                hours, (hours > 1 ? "h" : ""));
    } else if (hours >= 2) {
        sprintf(result, "%d%s %d%s", 
                hours, (hours > 0 ? "h" : ""), 
                minutes, (minutes > 1 ? "m" : ""));
    } else {
        sprintf(result, "%d%s %d%s", 
                minutes, (minutes > 0 ? "m" : ""), 
                seconds, (seconds > 1 ? "s" : ""));
    }

    Serial.printf("getUptime(%i) -> %s\n", millis() / 1000UL, result);
}
