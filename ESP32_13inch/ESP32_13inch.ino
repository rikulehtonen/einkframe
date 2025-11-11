#include "EPD_13in3e.h"
#include "GUI_Paint.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"

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

//Uncomment and set up if you want to use custom pins for the SPI communication
#define REASSIGN_PINS
int sck = 18;
int miso = 19;
int mosi = 23;
int cs = 12;

void setup() {

    //while (!Serial) {
    //    ; // wait for serial port to connect. Needed for native USB port only
    //}
	Serial.begin(115200);

    Serial.print("Initializing SD card...");

    SPI.begin(sck, miso, mosi, cs);
    if (!SD.begin(cs)) {
        Debug("initialization failed!");
        return;
    }
    Debug("initialization done.");


    String file_name = getNextFile();
    Serial.println(file_name);
    //render(file_name);
    //Serial.println("Sleeping");
    delay(1000UL * 60 * 4);
    //Serial.println("Sleeping complete");

}


String getNextFile() {
    int config_index = 0;
    String file_name = "";

    // Read index from config.txt
    File myFile = SD.open("config.txt", FILE_READ);
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
    myFile = SD.open("config.txt", FILE_READ);
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


void loop() { }