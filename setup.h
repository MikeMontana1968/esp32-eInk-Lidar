#define I2C_SDA GPIO_NUM_39
#define I2C_SCL GPIO_NUM_38

const int   VLX_SAMPLE_SIZE = 10;
const int   MS_DELAY_PER_SAMPLE = 100;

const int   FULL_TANK_MM = 15;
const int   EMPTY_TANK_MM = (7 * 25.4);
const float TANK_GALLONS = 4.5;
const int   MPG_AVG = 40;
const int   UPDATE_REFRESH_SEC = 30;

const char* TITLE = "2012 Victory Highball";