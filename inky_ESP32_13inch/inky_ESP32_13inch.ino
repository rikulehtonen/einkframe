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
//#include "GUI_Paint.h"
#include "Debug.h"
#include "DEV_Config.h"

#include "FS.h"
#include "SD.h"
#include "SPI.h"

#define uS_TO_S_FACTOR 1000000ULL /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  60*60*13          /* Time ESP32 will go to sleep (in seconds) */

// Uncomment and set up if you want to use custom pins for the SPI communication
int sck = 18;
int miso = 19;
int mosi = 23;
int cs = 17;

File f;
int g_current_file_index = 0;  // Global to track which file number we're on

void setup() {
    Serial.begin(115200);
    DEV_Delay_ms(3000);
    Debug("EPD_13IN3E_test Demo\r\n");
    DEV_Module_Init();
    EPD_13IN3E_Init();
    int sd_init_retries = 0;
    const int MAX_SD_RETRIES = 5;
    const int SD_RETRY_DELAY = 500; // milliseconds
    

    while (!SD.begin(cs) && sd_init_retries < MAX_SD_RETRIES) {
        Serial.printf("SD Card Mount Failed (attempt %d/%d), retrying...\n", sd_init_retries + 1, MAX_SD_RETRIES);
        DEV_Delay_ms(SD_RETRY_DELAY);
        sd_init_retries++;
    }

    if (sd_init_retries >= MAX_SD_RETRIES) {
        Serial.println("Card Mount Failed");
    }
    else {
        // Get and print the next file
        String nextFile = getNextFile();
        Serial.printf("Processing file: %s\n", nextFile.c_str());
        // Clear screen every 5 files (index 0, 5, 10, etc.)
        if (g_current_file_index % 5 == 0) {
            EPD_13IN3E_Clear(EPD_13IN3E_WHITE);
        }
        render("/" + nextFile);
    }

    Debug("Goto Sleep...\r\n");
    EPD_13IN3E_Sleep();
    // close 5V
    Debug("close 5V, Module enters 0 power consumption ...\r\n");
    DEV_Module_Exit();
    
    //Deep sleep
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
    Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) + " Seconds");
    Serial.println("Going to sleep now");
    Serial.flush();
    esp_deep_sleep_start();

}

String getNextFile() {
    int config_index = 0;
    String file_name = "";

    // Read index from config.txt
    File f = SD.open("/config.txt", FILE_READ);
    if (f) {
        String index_string = "";
        while (f.available()) {
            index_string += (char)f.read();
        }
        f.close();
        config_index = index_string.toInt();
    } else {
        Serial.println("Error opening config.txt for reading");
    }

    // Open files list
    f = SD.open("/files.txt", FILE_READ);
    if (f) {
        int current_file_index = 0;
        while (f.available() && current_file_index <= config_index) {
            file_name = f.readStringUntil(',');
            current_file_index++;
        }

        // If we've reached the end, wrap around
        if (!f.available()) {
            config_index = 0;
        } else {
            config_index++;
        }

        f.close();
    } else {
        Serial.println("Error opening files.txt");
    }

    // Write back updated index
    SD.remove("/config.txt");
    f = SD.open("/config.txt", FILE_WRITE);
    if (f) {
        f.print(config_index);
        f.close();
    } else {
        Serial.println("Error opening config.txt for writing");
    }

    // Store the current index globally
    g_current_file_index = config_index;

    // Print the next file name
    Serial.printf("Next file: %s\n", file_name.c_str());
    return file_name;
}


void render(String file_name){
    
    UDOUBLE Width, Width1, Height;
    UBYTE Color;
    Width = (EPD_13IN3E_WIDTH % 2 == 0)? (EPD_13IN3E_WIDTH / 2 ): (EPD_13IN3E_WIDTH / 2 + 1);
    Width1 = (Width % 2 == 0)? (Width / 2 ): (Width / 2 + 1);
    Height = EPD_13IN3E_HEIGHT;
    Color = (EPD_13IN3E_WHITE<<4)|EPD_13IN3E_WHITE;

    UBYTE padding[Width1];
    
    for (UDOUBLE j = 0; j < Width/2; j++) {
        padding[j] = Color;
    }

    DEV_Delay_ms(5000);
    Debug("EPD_13IN3E_test Demo\r\n");
    //EPD_13IN3E_Clear(EPD_13IN3E_WHITE);

    if(psramInit()){
        Serial.println("\nPSRAM is correctly initialized");
    }else{
        Serial.println("PSRAM not available");
    }

    UDOUBLE n = 0;
    // Read index from config.txt
    UBYTE *buf = (UBYTE *)ps_malloc((Width)*EPD_13IN3E_HEIGHT);

    File f = SD.open(file_name, FILE_READ);
    if(f.available()) {
        Serial.print("Buffer contents: ");
        for (UDOUBLE j = 0; j < EPD_13IN3E_HEIGHT; j++) {
            size_t bytesRead = f.read(buf + j*Width, Width);
        }
        //for (UDOUBLE i = 0; i < Width; i++) {
        //    Serial.printf("0x%02X ", buf[i]);
        //}
        //Serial.println();
    } else {
        Serial.println("FAILED");
    }
    f.close();


    DEV_Digital_Write(EPD_CS_M_PIN, 0);
    EPD_13IN3E_SendCommand(0x10);
    for (UDOUBLE j=0; j<Height; j++) {
        EPD_13IN3E_SendData2(buf + j*Width, Width1);
        DEV_Delay_ms(1);
    }
    EPD_13IN3E_CS_ALL(1);

    DEV_Digital_Write(EPD_CS_S_PIN, 0);
    EPD_13IN3E_SendCommand(0x10);
    for (UDOUBLE j=0; j<Height; j++) {
        EPD_13IN3E_SendData2(buf + j*Width + Width1, Width1);
        DEV_Delay_ms(1);
    }
    EPD_13IN3E_CS_ALL(1);
    DEV_Delay_ms(1000);


    EPD_13IN3E_TurnOnDisplay();
    free(buf);
}

void loop() {}