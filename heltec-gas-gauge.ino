#include <heltec-eink-modules.h>
#include "Fonts/FreeSans9pt7b.h"
#include "Fonts/FreeSansBold9pt7b.h"


/*
Set Board Type to: Vision Master E290
https://github.com/todd-herbert/heltec-eink-modules/blob/main/docs/WirelessPaper/wireless_paper.md

*/
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 3600;
char BUFFER[255] = {0};

EInkDisplay_VisionMasterE290 display;

void setup() {
	Serial.begin(115200);
    delay(200);
    Serial.print("Running ");
    Serial.println(__FILENAME__);
    Serial.print(" Version: ");
    Serial.println(__TIMESTAMP__);
	
    display.print("Hello, world!");
    display.update();
    
    delay(4000);
    display.clearMemory();  // Start a new drawing

    display.landscape();
    display.setFont( &FreeSans9pt7b );
    display.printCenter("In the middle.");
    display.update();

    delay(4000);
    display.clearMemory();  // Start a new drawing

    display.setCursor(5, 20);   // Text Cursor - x:5 y:20
    display.println("Here's a normal update.");
    display.update();

    delay(2000);    // Pause to read

    display.fastmodeOn();
    display.println("Here's fastmode,");
    display.update();
    display.println("aka Partial Refresh.");
    display.update();

    delay(2000);    // Pause to read
    
    display.fastmodeOff();
    display.setFont( &FreeSansBold9pt7b );  // Change to bold font
    display.println();
    display.println("Don't use too often!");
    display.update();

    delay(4000);
    display.clear();    // Physically clear the screen

    display.fastmodeOn();
    display.setFont( NULL ); // Default Font
    display.printCenter("How about a dot?", 0, -40);   // 40px above center
    display.update();

    delay(2000);

    display.fastmodeOff();
    display.fillCircle(
        display.centerX(),  // X: center screen
        display.centerY(),  // Y: center screen
        10,                             // Radius: 10px
        BLACK                           // Color: black
        );
    display.update();

    delay(2000);
    
    display.fillCircle(display.centerX(), display.centerY(), 5, WHITE);   // Draw a smaller white circle inside, giving a "ring" shape
    display.update();

    delay(10000);
    display.clear();    // Physically clear the screen, ready for your next masterpiece
}

void loop() {

}