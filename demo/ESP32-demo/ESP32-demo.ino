#include "EPD_13in3e.h"
#include "GUI_Paint.h"
#include "fonts.h"
#include "ImageData.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"

UBYTE bmpColorToEPD(uint8_t r, uint8_t g, uint8_t b) {
    if (r < 50 && g < 50 && b < 50) return EPD_13IN3E_BLACK;     // black
    if (r > 200 && g > 200 && b > 200) return EPD_13IN3E_WHITE;  // white
    if (r > 200 && g > 200 && b < 80) return EPD_13IN3E_YELLOW;  // yellow
    if (r > 200 && g < 80 && b < 80) return EPD_13IN3E_RED;      // red
    if (r < 80 && g < 80 && b > 200) return EPD_13IN3E_BLUE;     // blue
    if (r < 80 && g > 200 && b < 80) return EPD_13IN3E_GREEN;    // green
    return EPD_13IN3E_WHITE; // fallback
}

// Uncomment and set up if you want to use custom pins for the SPI communication
int sck = 18;
int miso = 19;
int mosi = 23;
int cs = 12; // SD card CS

int eink_cs_m = 2;   // EINK Master CS (from DEV_Config.h)
int eink_cs_s = 13;  // EINK Slave CS (from DEV_Config.h)

File myFile;

void setup() {
    // Ensure all CS pins are HIGH before SPI begin

    UDOUBLE Width, Height;
    UBYTE Color;
    Width = (EPD_13IN3E_WIDTH % 2 == 0)? (EPD_13IN3E_WIDTH / 2 ): (EPD_13IN3E_WIDTH / 2 + 1);
    Height = EPD_13IN3E_HEIGHT;
    Color = (EPD_13IN3E_WHITE<<4)|EPD_13IN3E_WHITE;
        
    UBYTE buf[Width/2];
    for (UDOUBLE j = 0; j < Width/2; j++) {
        buf[j] = Color;
    }

    Debug("EPD_13IN3E_test Demo\r\n");
    DEV_Module_Init();

    SPI.begin(sck, miso, mosi);
    if (!SD.begin(cs)) {
        Serial.println("Card Mount Failed");
        return;
    }

    // Get and print the next file
    String nextFile = getNextFile();
    Serial.printf("Processing file: %s\n", nextFile.c_str());

    UDOUBLE n = 0;
    // Read index from config.txt
    File myFile = SD.open("/" + nextFile, FILE_READ);
    UDOUBLE j = 0;

    if(myFile.available()) {
        for (UDOUBLE j = 0; j < Width/2; j++) {
            buf[j] = myFile.read();
        }
            
        // Print the buffer contents
        Serial.print("Buffer contents: ");
        for (UDOUBLE j = 0; j < Width/2; j++) {
            Serial.printf("0x%02X ", buf[j]);
        }
        Serial.println();
    } else {
        Serial.println("FAILED");
    }


    //Debug("EPD_13IN3E_test Demo\r\n");
    DEV_Module_Init();
    //Debug("e-Paper Init...\r\n");
    EPD_13IN3E_Init();
    pinMode(cs, OUTPUT);
    pinMode(eink_cs_m, OUTPUT);
    pinMode(eink_cs_s, OUTPUT);
    digitalWrite(cs, HIGH);
    digitalWrite(eink_cs_m, LOW);
    digitalWrite(eink_cs_s, LOW);
    EPD_13IN3E_Clear(EPD_13IN3E_WHITE);


    if(myFile.available()) {
        for (UDOUBLE j = 0; j < Width/2; j++) {
            buf[j] = myFile.read();
        }  
        // Print the buffer contents
        Serial.print("Buffer contents: ");
        for (UDOUBLE j = 0; j < Width/2; j++) {
            Serial.printf("0x%02X ", buf[j]);
        }
        Serial.println();
    } else {
        Serial.println("FAILED");
    }

    Debug("Goto Sleep...\r\n");
    EPD_13IN3E_Sleep();
    // close 5V
    Debug("close 5V, Module enters 0 power consumption ...\r\n");
    DEV_Module_Exit();
}




String getNextFile() {
    int config_index = 0;
    String file_name = "";

    // Read index from config.txt
    File myFile = SD.open("/config.txt", FILE_READ);
    if (myFile) {
        String index_string = "";
        while (myFile.available()) {
            index_string += (char)myFile.read();
        }
        myFile.close();
        config_index = index_string.toInt();
    } else {
        Serial.println("Error opening config.txt for reading");
    }

    // Open files list
    myFile = SD.open("/files.txt", FILE_READ);
    if (myFile) {
        int current_file_index = 0;
        while (myFile.available() && current_file_index <= config_index) {
            file_name = myFile.readStringUntil(',');
            current_file_index++;
        }

        // If we've reached the end, wrap around
        if (!myFile.available()) {
            config_index = 0;
        } else {
            config_index++;
        }

        myFile.close();
    } else {
        Serial.println("Error opening files.txt");
    }

    // Write back updated index
    SD.remove("/config.txt");
    myFile = SD.open("/config.txt", FILE_WRITE);
    if (myFile) {
        myFile.print(config_index);
        myFile.close();
    } else {
        Serial.println("Error opening config.txt for writing");
    }

    // Print the next file name
    Serial.printf("Next file: %s\n", file_name.c_str());
    return file_name;
}

void loop() {

}