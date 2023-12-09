#include <Arduino.h>
#include <SPI.h>
#include "epd4in2.h"
#include "imagedata.h"
#include "epdpaint.h"

#define COLORED     0
#define UNCOLORED   1

void setup() {

    // put your setup code here, to run once:
    Serial.begin(9600);
    /* while(!Serial.available()); */

    Serial.println("Starting epaper");

    Epd epd;

    if (epd.Init() != 0) {
        Serial.print("e-Paper init failed");
        return;
    }
    //Serial.print(UNCOLORED);
    /* This clears the SRAM of the e-paper display */
    epd.ClearFrame();

    unsigned char image[15000];
    Paint paint(image, 400, 300);    //width should be the multiple of 8 

    paint.Clear(UNCOLORED);
    /* paint.DrawStringAt(0, 0, "e-Paper Demo", &Font24, COLORED); */

    paint.DrawFilledRectangle(190,0,210,300, COLORED);

    sFONT *font = &Font72;
    paint.DrawStringAt(100 - font->Width/2, 150 - font->Height/2, "3", font, COLORED);
    paint.DrawStringAt(300 - font->Width/2, 150 - font->Height/2, "4", font, COLORED);


    Serial.println("Displaying");
    epd.DisplayFrame(paint.GetImage(), true);

    epd.Sleep();
}

void loop() {

    Serial.println("BLINK");
    delay(1000);
}
