#include "EPD_4in0e.h"
#include "GUI_Paint.h"
#include <SPI.h>
#include <SD.h>
//#include "fonts.h"
//#include "ImageData.h"

File myFile;

// Convert RGB to nearest supported e-paper color
UBYTE bmpColorToEPD(uint8_t r, uint8_t g, uint8_t b) {
    if (r < 50 && g < 50 && b < 50) return EPD_4IN0E_BLACK;     // black
    if (r > 200 && g > 200 && b > 200) return EPD_4IN0E_WHITE;  // white
    if (r > 200 && g > 200 && b < 80) return EPD_4IN0E_YELLOW;  // yellow
    if (r > 200 && g < 80 && b < 80) return EPD_4IN0E_RED;      // red
    if (r < 80 && g < 80 && b > 200) return EPD_4IN0E_BLUE;     // blue
    if (r < 80 && g > 200 && b < 80) return EPD_4IN0E_GREEN;    // green
    return EPD_4IN0E_WHITE; // fallback
}

void setup() {

    while (!Serial) {
        ; // wait for serial port to connect. Needed for native USB port only
    }
	Serial.begin(115200);

    Serial.print("Initializing SD card...");

    if (!SD.begin(4)) {
        Debug("initialization failed!");
        while (1);
    }
    Debug("initialization done.");
}


void loop() {
    String file_name = getNextFile();
    render(file_name);
    //Serial.println("Sleeping");
    delay(1000UL * 60 * 4);
    //Serial.println("Sleeping complete");
}


String getNextFile() {
    int config_index = 0;
    String file_name = "";

    // Read index from config.txt
    myFile = SD.open("config.txt");
    if (myFile) {
        String index_string = "";
        while (myFile.available()) {
            index_string += (char)myFile.read();
        }
        myFile.close();
        config_index = index_string.toInt();
    } else {
        Serial.println("error opening config.txt");
    }

    // Open files list
    myFile = SD.open("files.txt");
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
        Serial.println("error opening files.txt");
    }

    // Write back updated index
    SD.remove("config.txt");
    myFile = SD.open("config.txt", FILE_WRITE);
    if (myFile) {
        myFile.print(config_index);
        myFile.close();
    } else {
        Serial.println("error opening config.txt for write");
    }

    return file_name;
}



void render(String file_name){
    
    Debug("EPD_4IN0E_test Demo\r\n");
    DEV_Module_Init();

    Debug("e-Paper Init and Clear...\r\n");
    EPD_4IN0E_Init();
    EPD_4IN0E_Clear(EPD_4IN0E_WHITE);
    DEV_Delay_ms(500);

    // unsigned long j, k;
    unsigned char const Color_seven[6] = 
    {EPD_4IN0E_BLACK, EPD_4IN0E_YELLOW, EPD_4IN0E_RED, EPD_4IN0E_BLUE, EPD_4IN0E_GREEN, EPD_4IN0E_WHITE};

    // re-open the file for reading:
    myFile = SD.open(file_name);
    if (myFile) {
        EPD_4IN0E_SendCommand(0x10);
        // read from the file until there's nothing else in it:
        int n = 0;
        while (myFile.available() && n < 50000) {
            const UBYTE c = myFile.read();
            if (c != -1) {                
                //Debug(c);
                //Debug(" ");
                EPD_4IN0E_SendData(c);
                n += 1;
            }
        }

        // close the file:
        myFile.close();
        // Render image to the display
        EPD_4IN0E_TurnOnDisplay();
    } else {
        // if the file didn't open, print an error:
        Debug("error opening image");
    }

    Debug("Goto Sleep...\r\n");
    EPD_4IN0E_Sleep();
    DEV_Delay_ms(2000);

    // close 5V
    Debug("close 5V, Module enters 0 power consumption ...\r\n");
    DEV_Module_Exit();
}