# Heltec Gas Gauge

An ESP32-based fuel tank monitoring system using a LIDAR sensor and e-ink display. Designed for vehicle fuel level monitoring with real-time display of fuel percentage, remaining gallons, estimated range, and historical consumption data.

## Hardware Requirements

### Microcontroller
- **Heltec Vision Master E290** (ESP32-based board with integrated e-ink display)
- 296x152 pixel e-ink display (3-color: black, white, red)

### Sensor
- **Adafruit VL6180X** Time-of-Flight LIDAR sensor
- Range: 0-255mm
- Mounted above fuel tank to measure fuel level depth

### Pin Configuration
| Function | GPIO Pin |
|----------|----------|
| I2C SDA  | GPIO 39  |
| I2C SCL  | GPIO 38  |

### Tank Configuration Defaults
| Parameter | Default Value |
|-----------|---------------|
| Full tank distance | 10mm |
| Empty tank distance | 152mm |
| Tank capacity | 4.5 gallons |
| Average MPG | 40 |
| Screen refresh interval | 30 seconds |

### 3D Printed Enclosure
STL files for a custom enclosure are included in the `/stl/` directory:
- `E290 Bottom.stl` - Bottom housing
- `E290 Top.stl` - Top housing
- `E290 Buttons.stl` - Button interface

## Dependencies

### Arduino Libraries
- `heltec-eink-modules` - Heltec e-ink display driver
- `Adafruit_VL6180X` - VL6180X LIDAR sensor driver
- `Adafruit_GFX` - Graphics library
- `Wire` - I2C communication

### Fonts (included with Adafruit_GFX)
- FreeSans9pt7b
- FreeSansBold9pt7b
- FreeSansBold12pt7b
- FreeMono9pt7b

### ESP32 Framework
- FreeRTOS (included with ESP32 Arduino core)
- ESP32 ADC driver
- ESP32 logging framework

## Build Process

### Using Arduino IDE

1. **Install Board Support**
   - Open Arduino IDE Preferences
   - Add Heltec ESP32 board URL to Additional Board Manager URLs:
     ```
     https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series/releases/download/0.0.9/package_heltec_esp32_index.json
     ```
   - Open Boards Manager and install "Heltec ESP32 Series Dev-boards"
   - Select board: **Heltec Vision Master E290**

2. **Install Libraries**
   - Open Library Manager (Sketch > Include Library > Manage Libraries)
   - Install:
     - `Adafruit VL6180X`
     - `Adafruit GFX Library`
     - `heltec-eink-modules`

3. **Compile and Upload**
   - Open `heltec-gas-gauge.ino`
   - Select the correct COM port
   - Click Upload

### Using PlatformIO

1. **Create/Update platformio.ini**
   ```ini
   [env:visionmaster_e290]
   platform = espressif32
   board = heltec_vision_master_e290
   framework = arduino
   monitor_speed = 115200
   lib_deps =
       adafruit/Adafruit VL6180X
       adafruit/Adafruit GFX Library
       heltec-eink-modules
   ```

2. **Build and Upload**
   ```powershell
   pio run --target upload
   ```

3. **Monitor Serial Output**
   ```powershell
   pio device monitor --baud 115200
   ```

## Architecture

The system uses FreeRTOS to run two independent tasks:

| Task | Priority | Function |
|------|----------|----------|
| VlxTask | 3 | Continuously samples LIDAR sensor (100 samples/cycle) |
| EinkDisplayTask | 3 | Updates e-ink display every 30 seconds |

### Data Flow
1. VlxTask reads 100 samples from VL6180X sensor
2. Calculates average distance and standard deviation
3. Stores result in 200-element ring buffer for history
4. EinkDisplayTask retrieves latest data via mutex-protected access
5. Display renders fuel gauge, statistics, and historical bar graph

## Display Features

- Current fuel level percentage with color bar
- Remaining fuel volume (gallons)
- Estimated driving range (miles)
- 200-sample historical fuel level graph
- Sensor uptime and status
- Error messages for sensor failures

## Customization

Tank parameters can be modified in `eink_display.cpp`:

```cpp
EinkDisplayTask(VlxTask *vlxTask,
                int8_t sda = -1,
                int8_t scl = -1,
                uint8_t tankFull_mm = 10,      // Distance when tank is full
                uint8_t tankEmpty_mm = 152,    // Distance when tank is empty
                float tankCapacity = 4.5,       // Tank capacity in gallons
                uint8_t avgMpg = 40,            // Average fuel economy
                uint16_t screenRefresh_sec = 30 // Display refresh interval
);
```

## License

See LICENSE file for details.
