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

    /**
     * Due to RAM not enough in Arduino UNO, a frame buffer is not allowed.
     * In this case, a smaller image buffer is allocated and you have to 
     * update a partial display several times.
     * 1 byte = 8 pixels, therefore you have to set 8*N pixels at a time.
     */
    unsigned char image[15000];
    Paint paint(image, 400, 300);    //width should be the multiple of 8 

    paint.Clear(UNCOLORED);
    paint.DrawStringAt(0, 0, "e-Paper Demo", &Font24, COLORED);
    /* epd.SetPartialWindow(paint.GetImage(), 100, 40, paint.GetWidth(), paint.GetHeight()); */

    /* paint.Clear(COLORED); */
    /* paint.DrawStringAt(100, 2, "Hello world", &Font24, UNCOLORED); */
    /* epd.SetPartialWindow(paint.GetImage(), 0, 64, paint.GetWidth(), paint.GetHeight()); */

    /* paint.SetWidth(64); */
    /* paint.SetHeight(64); */

    /* paint.Clear(UNCOLORED); */
    /* paint.DrawRectangle(0, 0, 40, 50, COLORED); */
    /* paint.DrawLine(0, 0, 40, 50, COLORED); */
    /* paint.DrawLine(40, 0, 0, 50, COLORED); */
    /* epd.SetPartialWindow(paint.GetImage(), 72, 120, paint.GetWidth(), paint.GetHeight()); */

    /* paint.Clear(UNCOLORED); */
    /* paint.DrawCircle(32, 32, 30, COLORED); */
    /* epd.SetPartialWindow(paint.GetImage(), 200, 120, paint.GetWidth(), paint.GetHeight()); */

    /* paint.Clear(UNCOLORED); */
    /* paint.DrawFilledRectangle(0, 0, 40, 50, COLORED); */
    /* epd.SetPartialWindow(paint.GetImage(), 72, 200, paint.GetWidth(), paint.GetHeight()); */

    /* paint.Clear(UNCOLORED); */
    /* paint.DrawFilledCircle(32, 32, 30, COLORED); */
    /* epd.SetPartialWindow(paint.GetImage(), 200, 200, paint.GetWidth(), paint.GetHeight()); */

    /* This displays the data from the SRAM in e-Paper module */
    Serial.println("Displaying");
    epd.DisplayFrame(paint.GetImage(), true);

    /* This displays an image */
    //epd.DisplayFrame(IMAGE_BUTTERFLY);



    /* epd.Init_4Gray(); */
    /* epd.ClearFrame(); */
    /* epd.Set_4GrayDisplay(gImage_4in2_4Gray1, 100, 100,  200,150); */
    /* delay(2000); */

    /* /1* Deep sleep *1/ */
    /* epd.ClearFrame(); */
    /* epd.DisplayFrame(); */
    epd.Sleep();

    /* pinMode(LED_BUILTIN, OUTPUT); */
}

void loop() {

    Serial.println("BLINK");
    Serial.println(digitalRead(7));
    /* digitalWrite(LED_BUILTIN, HIGH); */
    /* delay(1000); */
    /* digitalWrite(LED_BUILTIN, LOW); */
    delay(1000);
}
