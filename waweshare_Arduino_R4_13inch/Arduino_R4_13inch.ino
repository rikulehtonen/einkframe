#include "EPD_13in3e.h"
#include <SPI.h>
#include <SD.h>
#include "Debug.h"

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
    
    Debug("EPD_13IN3E_test Demo\r\n");
    DEV_Module_Init();

    Debug("e-Paper Init and Clear...\r\n");
    EPD_13IN3E_Init();
    EPD_13IN3E_Clear(EPD_13IN3E_WHITE);
    DEV_Delay_ms(500);

    // unsigned long j, k;
    unsigned char const Color_seven[6] = 
    {EPD_13IN3E_BLACK, EPD_13IN3E_YELLOW, EPD_13IN3E_RED, EPD_13IN3E_BLUE, EPD_13IN3E_GREEN, EPD_13IN3E_WHITE};

    // re-open the file for reading:
    myFile = SD.open(file_name);
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

        UDOUBLE n = 0;
        while (myFile.available() && n < Height*2) {

            for (UDOUBLE j = 0; j < Width/2; j++) {
                if (myFile.available()) {
                    buf[j] = myFile.read();
                }
                buf[j] = Color;
            }
            
            if (n%2) { DEV_Digital_Write(EPD_CS_M_PIN, 0); }
            else { DEV_Digital_Write(EPD_CS_S_PIN, 0); }
            EPD_13IN3E_SendCommand(0x10);
            EPD_13IN3E_SendData2(buf, Width/2);
            EPD_13IN3E_CS_ALL(1);
            n+=1;
        }

        // close the file:
        myFile.close();
        // Render image to the display
        EPD_13IN3E_TurnOnDisplay();
    } else {
        // if the file didn't open, print an error:
        Debug("error opening image");
    }

    Debug("Goto Sleep...\r\n");
    EPD_13IN3E_Sleep();
    DEV_Delay_ms(2000);

    // close 5V
    Debug("close 5V, Module enters 0 power consumption ...\r\n");
    DEV_Module_Exit();
}