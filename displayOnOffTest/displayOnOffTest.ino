/*
 * This is an example sketch that shows how to toggle the SSD1331 OLED display
 * on and off at runtime to avoid screen burn-in.
 * 
 * The sketch also demonstrates how to erase a previous value by re-drawing the 
 * older value in the screen background color prior to writing a new value in
 * the same location. This avoids the need to call fillScreen() to erase the
 * entire screen followed by a complete redraw of screen contents.
 * 
 * Written by Phill Kelley. BSD license.
 */
 
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
#include <SPI.h>
#include "RTClib.h"

#define SerialDebugging true

// Screen dimensions
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 128 // Change this to 96 for 1.27" OLED.

RTC_Millis rtc;
DateTime targetDate(2025, 8, 10, 8, 0, 0);

const uint8_t   status_pin = 5;

// The SSD1351 is connected like this (plus VCC plus GND)
const uint8_t   OLED_pin_scl_sck        = 13;
const uint8_t   OLED_pin_sda_mosi       = 11;
const uint8_t   OLED_pin_cs_ss          = 10;
const uint8_t   OLED_pin_res_rst        = 9;
const uint8_t   OLED_pin_dc_rs          = 8;

// connect a push button to ...
const uint8_t   Button_pin              = 2;

// SSD1331 color definitions
const uint16_t  OLED_Color_Black        = 0x0000;
const uint16_t  OLED_Color_Blue         = 0x001F;
const uint16_t  OLED_Color_Red          = 0xF800;
const uint16_t  OLED_Color_Green        = 0x07E0;
const uint16_t  OLED_Color_Cyan         = 0x07FF;
const uint16_t  OLED_Color_Magenta      = 0xF81F;
const uint16_t  OLED_Color_Yellow       = 0xFFE0;
const uint16_t  OLED_Color_White        = 0xFFFF;

// The colors we actually want to use
uint16_t        OLED_Text_Color         = OLED_Color_White;
uint16_t        OLED_Backround_Color    = OLED_Color_Black;

uint8_t previous_minute = 60;

// declare the display
Adafruit_SSD1351 oled =
    Adafruit_SSD1351(
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        &SPI,
        OLED_pin_cs_ss,
        OLED_pin_dc_rs,
        OLED_pin_res_rst
     );

// assume the display is off until configured in setup()
bool            isDisplayVisible        = false;

// declare size of working string buffers. Basic strlen("d hh:mm:ss") = 10
const size_t    MaxString               = 32;

// the string being displayed on the SSD1331 (initially empty)
char oldTimeString[MaxString]           = { 0 };
char oldMonthsString[MaxString] = { 0 };

// the interrupt service routine affects this
volatile bool   isButtonPressed         = false;

// interrupt service routine
void senseButtonPressed() {
    if (!isButtonPressed) {
        isButtonPressed = true;
    }
}

void showDate(const char* txt, const DateTime& dt) {
    Serial.print(txt);
    Serial.print(' ');
    Serial.print(dt.year(), DEC);
    Serial.print('/');
    Serial.print(dt.month(), DEC);
    Serial.print('/');
    Serial.print(dt.day(), DEC);
    Serial.print(' ');
    Serial.print(dt.hour(), DEC);
    Serial.print(':');
    Serial.print(dt.minute(), DEC);
    Serial.print(':');
    Serial.print(dt.second(), DEC);

    Serial.print(" = ");
    Serial.print(dt.unixtime());
    Serial.print("s / ");
    Serial.print(dt.unixtime() / 86400L);
    Serial.print("d since 1970");

    Serial.println();
}

void displayUpTime() {

    // Start date for Geordie August 10 2025
    DateTime now = rtc.now();

    DateTime timeRemaining (targetDate.unixtime() - now.unixtime());

    if (timeRemaining.minute() < previous_minute) {
        digitalWrite(status_pin, HIGH);
        previous_minute = timeRemaining.minute();
    }
    delay(200);
    digitalWrite(status_pin, LOW);


    Serial.print("Months:");Serial.println(timeRemaining.month());
    Serial.print("Days:");Serial.println(timeRemaining.day());
    Serial.print("hours:");Serial.println(timeRemaining.hour());
    Serial.print("minutes:");Serial.println(timeRemaining.minute());

    // calculate seconds, truncated to the nearest whole second
    unsigned long upSeconds = millis() / 1000;

    // calculate days, truncated to nearest whole day
    unsigned long days = upSeconds / 86400;

    // the remaining hhmmss are
    upSeconds = upSeconds % 86400;

    // calculate hours, truncated to the nearest whole hour
    unsigned long hours = upSeconds / 3600;

    // the remaining mmss are
    upSeconds = upSeconds % 3600;

    // calculate minutes, truncated to the nearest whole minute
    unsigned long minutes = upSeconds / 60;

    // the remaining ss are
    upSeconds = upSeconds % 60;

    // allocate a buffer
    char newTimeString[MaxString] = { 0 };
    char newTitleString[MaxString] = { 0 };

    char monthString[MaxString] = { 0 };
    char dayString[MaxString] = { 0 };
    char hourString[MaxString] = { 0 };

    // construct the string representation
    sprintf(
        newTimeString,
        "%lu %02lu:%02lu:%02lu",
        days, hours, minutes, upSeconds
    );
    sprintf(newTitleString,"%s", "Roll Tide!");
    sprintf(monthString, "%lu mo %lu dy %lu hr %lu m", timeRemaining.month(), timeRemaining.day(), timeRemaining.hour(), timeRemaining.minute());
    sprintf(dayString, "%lu", timeRemaining.day());
    sprintf(hourString, "%lu", timeRemaining.hour());

    Serial.print("strcmp(monthString,oldMonthsString):");Serial.println(strcmp(monthString,oldMonthsString));
    Serial.print("monthString:");Serial.println(monthString);
    Serial.print("oldMonthsString:");Serial.println(oldMonthsString);


    // has the time string changed since the last oled update?
    if (strcmp(monthString,oldMonthsString) != 0) {

        // yes! home the cursor
        oled.setCursor(0,0);

        // change the text color to the background color
        oled.setTextColor(OLED_Backround_Color);

        // home the cursor
        oled.setCursor(6,50);
        oled.setTextSize(2);
        // change the text color to foreground color
        oled.setTextColor(OLED_Text_Color);
    
        // draw the new time value
        oled.print(newTitleString);

        oled.setCursor(2, 80);
        oled.setTextSize(1);

        // change the text color to the background color
        oled.setTextColor(OLED_Backround_Color);

        // redraw the old value to erase
        oled.print(oldMonthsString);

        oled.setCursor(2, 80);
        oled.setTextSize(1);
        // change the text color to foreground color
        oled.setTextColor(OLED_Text_Color);
        oled.print(monthString);
        Serial.print("monthString:");
        Serial.println(monthString);
        // and remember the new value
        strcpy(oldMonthsString,monthString);
        
    }

}


void setup() {

    // button press pulls pin LOW so configure HIGH
    pinMode(Button_pin,INPUT_PULLUP);
    pinMode(status_pin, OUTPUT);

    // use an interrupt to sense when the button is pressed
    attachInterrupt(digitalPinToInterrupt(Button_pin), senseButtonPressed, FALLING);

    #if (SerialDebugging)
    Serial.begin(115200); while (!Serial); Serial.println();
    #endif

    // settling time
    delay(250);

    rtc.begin(DateTime(F(__DATE__), F(__TIME__)));

    showDate("startDate", targetDate);
    // ignore any power-on-reboot garbage
    isButtonPressed = false;

    // initialise the SSD1331
    oled.begin();
    oled.setFont();
    oled.fillScreen(OLED_Backround_Color);
    oled.setTextColor(OLED_Text_Color);
    oled.setTextSize(2);

    // the display is now on
    isDisplayVisible = true;

}


void loop() {

    // unconditional display, regardless of whether display is visible
    displayUpTime();

    // has the button been pressed?
    if (isButtonPressed) {
        
        // yes! toggle display visibility
        isDisplayVisible = !isDisplayVisible;

        // apply
        oled.enableDisplay(isDisplayVisible);

        #if (SerialDebugging)
        Serial.print("button pressed @ ");
        Serial.print(millis());
        Serial.print(", display is now ");
        Serial.println((isDisplayVisible ? "ON" : "OFF"));
        #endif

        // confirm button handled
        isButtonPressed = false;
        
    }

    // no need to be in too much of a hurry
    delay(10000);
   
}
