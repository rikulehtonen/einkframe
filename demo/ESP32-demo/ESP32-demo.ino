#include "EPD_13in3e.h"
// #include "GUI_Paint.h"
// #include "fonts.h"
// #include "ImageData.h"
#include "Debug.h"
#include "DEV_Config.h"

#include "FS.h"
#include "SD.h"
#include "SPI.h"

// Uncomment and set up if you want to use custom pins for the SPI communication
int sck = 18;
int miso = 19;
int mosi = 23;
int cs = 17; // SD card CS

int eink_cs_m = 2;   // EINK Master CS (from DEV_Config.h)
int eink_cs_s = 13;  // EINK Slave CS (from DEV_Config.h)

File myFile;

void setup() {
    // Ensure all CS pins are HIGH before SPI begin

    Serial.begin(115200);

    UDOUBLE Width, Height;
    UBYTE Color;
    Width = (EPD_13IN3E_WIDTH % 2 == 0)? (EPD_13IN3E_WIDTH / 2 ): (EPD_13IN3E_WIDTH / 2 + 1);
    Height = EPD_13IN3E_HEIGHT;
    Color = (EPD_13IN3E_RED<<4)|EPD_13IN3E_RED;

    Debug("EPD_13IN3E_test Demo\r\n");

        // Initialize EPD then (re)initialise hardware SPI and mount the SD card
    DEV_Module_Init();
    EPD_13IN3E_Init();
    EPD_13IN3E_Clear(EPD_13IN3E_WHITE);


    if (!SD.begin(cs)) {
        Serial.println("Card Mount Failed");

        Debug("Goto Sleep...\r\n");
        EPD_13IN3E_Sleep();
        // close 5V
        Debug("close 5V, Module enters 0 power consumption ...\r\n");
        DEV_Module_Exit();
        return;
    }
    else {
        Serial.println("SD card mounted");
    }


    UDOUBLE n = 0;
    // Read index from config.txt
    File f = SD.open("/test.bin", FILE_READ);
    UBYTE buf[Width/2];

    if(f.available()) {
        size_t bytesRead = f.read(buf, Width/2);
        Serial.print("Buffer contents: ");
        for (UDOUBLE i = 0; i < Width/2; i++) {
            Serial.printf("0x%02X ", buf[i]);
        }
        Serial.println();
    } else {
        Serial.println("FAILED");
    }



    DEV_Digital_Write(EPD_CS_M_PIN, 0);
    EPD_13IN3E_SendCommand(0x10);

    for (UDOUBLE j = 0; j < EPD_13IN3E_HEIGHT; j++) {
        if(true) {
            
            
            
            EPD_13IN3E_SendData2(buf, Width/2);
            DEV_Delay_ms(5);
        //Print the buffer contents
            if(j % 100 == 0) {
                Serial.print("Buffer contents: ");
                for (UDOUBLE i = 0; i < Width/2; i++) {
                    Serial.printf("0x%02X ", buf[i]);
                }
                Serial.println();
            }
        } else {
            Serial.println("FAILED");
        }
    }

    EPD_13IN3E_CS_ALL(1);

    f.close();

    EPD_13IN3E_TurnOnDisplay();

    Debug("Goto Sleep...\r\n");
    EPD_13IN3E_Sleep();
    // close 5V
    Debug("close 5V, Module enters 0 power consumption ...\r\n");
    DEV_Module_Exit();
}


void loop() {

}