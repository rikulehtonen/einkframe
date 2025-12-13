/*
 * SD Card Connections:
 * pin 1 - not used          |  Micro SD card     |
 * pin 2 - CS (SS)           |                   /
 * pin 3 - DI (MOSI)         |                  |__
 * pin 4 - VDD (3.3V)        |                    |
 * pin 5 - SCK (SCLK)        | 8 7 6 5 4 3 2 1   /
 * pin 6 - VSS (GND)         | ▄ ▄ ▄ ▄ ▄ ▄ ▄ ▄  /
 * pin 7 - DO (MISO)         | ▀ ▀ █ ▀ █ ▀ ▀ ▀ |
 * pin 8 - not used          |_________________|
 * 
 * Note: Use `SPI.begin(sck, miso, mosi, cs)` to manually configure SPI pins.
 */

#include "EPD_13in3e.h"
#include "GUI_Paint.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"

// Uncomment and set up if you want to use custom pins for the SPI communication
int sck = 18;
int miso = 19;
int mosi = 23;
int cs = 12;

File myFile;

// Convert RGB to nearest supported e-paper color
UBYTE bmpColorToEPD(uint8_t r, uint8_t g, uint8_t b) {
    if (r < 50 && g < 50 && b < 50) return EPD_13IN3E_BLACK;     // black
    if (r > 200 && g > 200 && b > 200) return EPD_13IN3E_WHITE;  // white
    if (r > 200 && g > 200 && b < 80) return EPD_13IN3E_YELLOW;  // yellow
    if (r > 200 && g < 80 && b < 80) return EPD_13IN3E_RED;      // red
    if (r < 80 && g < 80 && b > 200) return EPD_13IN3E_BLUE;     // blue
    if (r < 80 && g > 200 && b < 80) return EPD_13IN3E_GREEN;    // green
    return EPD_13IN3E_WHITE; // fallback
}

void setup() {

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
    render("/" + nextFile);
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

void render(String file_name){
    
    //EPD_13IN3E_CS_ALL(0);

    // unsigned long j, k;
    unsigned char const Color_seven[6] = 
    {EPD_13IN3E_BLACK, EPD_13IN3E_YELLOW, EPD_13IN3E_RED, EPD_13IN3E_BLUE, EPD_13IN3E_GREEN, EPD_13IN3E_WHITE};

    // re-open the file for reading:
    myFile = SD.open(file_name, FILE_READ);
    if (myFile) {


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
        Debug("e-Paper Init...\r\n");
        EPD_13IN3E_Init();
        EPD_13IN3E_Clear(EPD_13IN3E_WHITE);
        //DEV_Delay_ms(500);

        UDOUBLE n = 0;
        while (myFile.available() && n < Height*2) {

            for (UDOUBLE j = 0; j < Width/2; j++) {
                if (myFile.available()) {
                    buf[j] = myFile.read();
                }
            }
            
            // Print the buffer contents
            Serial.print("Buffer contents: ");
            for (UDOUBLE j = 0; j < Width/2; j++) {
                Serial.printf("0x%02X ", buf[j]);
            }
            Serial.println();

            EPD_13IN3E_CS_ALL(0);
            if (n%2) { DEV_Digital_Write(EPD_CS_S_PIN, 1); }
            else { DEV_Digital_Write(EPD_CS_M_PIN, 1); }
            EPD_13IN3E_SendCommand(0x10);
            EPD_13IN3E_SendData2(buf, Width/2);
            EPD_13IN3E_CS_ALL(0);
            n+=1;
        }

        // close the file:
        myFile.close();
        // Render image to the display
        EPD_13IN3E_TurnOnDisplay();
    } else {
        // if the file didn't open, print an error:
        Debug("error opening image\r\n");
    }

    Debug("Goto Sleep...\r\n");
    EPD_13IN3E_Sleep();
    DEV_Delay_ms(2000);

    // close 5V
    Debug("close 5V, Module enters 0 power consumption ...\r\n");
    DEV_Module_Exit();
}

void loop() {}