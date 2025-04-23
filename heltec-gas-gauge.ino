#include <heltec-eink-modules.h>

/*

https://github.com/todd-herbert/heltec-eink-modules/blob/main/docs/WirelessPaper/wireless_paper.md

*/

//EInkDisplay_WirelessPaperV1_1 display;
EInkDisplay_WirelessPaperV1 display;

void setup() {
    display.landscape();

    display.print("Hello, World!");
    display.update();

    delay(2000);
    
    display.clearMemory();
    display.printCenter("Centered Text");
    display.update();

    delay(2000);

    display.clear();
}

void loop() {

}