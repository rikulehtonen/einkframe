/*
 * simple_update.ino
 * Direct C++ port of simple_update.py for FireBeetle 2 (ESP32).
 * Runs the full init sequence then triggers a display refresh
 * without sending any pixel data.
 */

#include <Arduino.h>
#include <SPI.h>

// --- Pin definitions ---
#define EPD_SCK_PIN     18
#define EPD_MOSI_PIN    23
#define EPD_MISO_PIN    19
#define EPD_CS_M_PIN    2
#define EPD_CS_S_PIN    13
#define EPD_DC_PIN      14
#define EPD_RST_PIN     0
#define EPD_BUSY_PIN    26
#define EPD_PWR_PIN     25

// --- CS select masks (mirrors Python CS0_SEL / CS1_SEL) ---
#define CS0_SEL  0b01
#define CS1_SEL  0b10
#define CS_BOTH  (CS0_SEL | CS1_SEL)

// --- Command bytes ---
#define EL133UF1_PSR            0x00
#define EL133UF1_PWR            0x01
#define EL133UF1_POF            0x02
#define EL133UF1_PON            0x04
#define EL133UF1_BTST_N         0x05
#define EL133UF1_BTST_P         0x06
#define EL133UF1_DRF            0x12
#define EL133UF1_PLL            0x30
#define EL133UF1_CDI            0x50
#define EL133UF1_TCON           0x60
#define EL133UF1_TRES           0x61
#define EL133UF1_ANTM           0x74
#define EL133UF1_AGID           0x86
#define EL133UF1_BUCK_BOOST_VDDN 0xB0
#define EL133UF1_TFT_VCOM_POWER  0xB1
#define EL133UF1_EN_BUF          0xB6
#define EL133UF1_BOOST_VDDP_EN   0xB7
#define EL133UF1_CCSET           0xE0
#define EL133UF1_PWS             0xE3
#define EL133UF1_CMD66           0xF0


// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/*
 * busy_wait: poll BUSY pin until HIGH (idle).
 * LOW = busy, HIGH = idle (same as C++ comment in original driver).
 * timeout_ms is a safety ceiling in milliseconds.
 */
static void busy_wait(uint32_t timeout_ms = 40000)
{
    Serial.println("busy_wait...");
    uint32_t start = millis();
    while (digitalRead(EPD_BUSY_PIN) == LOW) {
        delay(10);
        if (millis() - start > timeout_ms) {
            Serial.println("busy_wait: timed out");
            return;
        }
    }
    delay(20);
    Serial.println("busy_wait: done");
}

/*
 * send_command: mirrors Python send_command(command, cs_sel, data).
 * Asserts the requested CS pin(s), pulls DC low, waits 300 ms,
 * sends the command byte, optionally sends data bytes with DC high,
 * then deasserts both CS pins and leaves DC low.
 */
static void send_command(uint8_t command, uint8_t cs_sel = CS_BOTH,
                         const uint8_t *data = nullptr, size_t data_len = 0)
{
    if (cs_sel & CS0_SEL) digitalWrite(EPD_CS_M_PIN, LOW);
    if (cs_sel & CS1_SEL) digitalWrite(EPD_CS_S_PIN, LOW);

    digitalWrite(EPD_DC_PIN, LOW);
    delay(300);  // required DC setup time before command byte

    SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE0));
    SPI.transfer(command);

    if (data != nullptr && data_len > 0) {
        digitalWrite(EPD_DC_PIN, HIGH);
        for (size_t i = 0; i < data_len; i++) {
            SPI.transfer(data[i]);
        }
    }
    SPI.endTransaction();

    // Always deassert both CS and reset DC low (matches Python behaviour)
    digitalWrite(EPD_CS_M_PIN, HIGH);
    digitalWrite(EPD_CS_S_PIN, HIGH);
    digitalWrite(EPD_DC_PIN, LOW);
}


// ---------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------

void setup()
{
    Serial.begin(115200);
    delay(500);
    Serial.println("simple_update start");

    // GPIO
    pinMode(EPD_BUSY_PIN, INPUT_PULLUP);
    pinMode(EPD_RST_PIN,  OUTPUT);
    pinMode(EPD_DC_PIN,   OUTPUT);
    pinMode(EPD_PWR_PIN,  OUTPUT);
    pinMode(EPD_CS_M_PIN, OUTPUT);
    pinMode(EPD_CS_S_PIN, OUTPUT);

    digitalWrite(EPD_CS_M_PIN, HIGH);
    digitalWrite(EPD_CS_S_PIN, HIGH);
    digitalWrite(EPD_DC_PIN,   LOW);
    digitalWrite(EPD_PWR_PIN,  HIGH);

    SPI.begin(EPD_SCK_PIN, EPD_MISO_PIN, EPD_MOSI_PIN);

    // --- Reset (mirrors Python: LOW -> 30ms -> HIGH -> 30ms) ---
    digitalWrite(EPD_RST_PIN, LOW);
    delay(30);
    digitalWrite(EPD_RST_PIN, HIGH);
    delay(30);

    Serial.println("sleepy");
    busy_wait(300);   // mirrors busy_wait(0.3)
    Serial.println("not sleepy");

    // --- Init sequence (exact match to simple_update.py) ---
    {
        const uint8_t d[] = {0xC0, 0x1C, 0x1C, 0xCC, 0xCC, 0xCC, 0x15, 0x15, 0x55};
        send_command(EL133UF1_ANTM, CS0_SEL, d, sizeof(d));
    }
    {
        const uint8_t d[] = {0x49, 0x55, 0x13, 0x5D, 0x05, 0x10};
        send_command(EL133UF1_CMD66, CS_BOTH, d, sizeof(d));
    }
    {
        const uint8_t d[] = {0xDF, 0x69};
        send_command(EL133UF1_PSR, CS_BOTH, d, sizeof(d));
    }
    {
        const uint8_t d[] = {0x08};
        send_command(EL133UF1_PLL, CS_BOTH, d, sizeof(d));
    }
    {
        const uint8_t d[] = {0xF7};
        send_command(EL133UF1_CDI, CS_BOTH, d, sizeof(d));
    }
    {
        const uint8_t d[] = {0x03, 0x03};
        send_command(EL133UF1_TCON, CS_BOTH, d, sizeof(d));
    }
    {
        const uint8_t d[] = {0x10};
        send_command(EL133UF1_AGID, CS_BOTH, d, sizeof(d));
    }
    {
        const uint8_t d[] = {0x22};
        send_command(EL133UF1_PWS, CS_BOTH, d, sizeof(d));
    }
    {
        const uint8_t d[] = {0x01};
        send_command(EL133UF1_CCSET, CS_BOTH, d, sizeof(d));
    }
    {
        const uint8_t d[] = {0x04, 0xB0, 0x03, 0x20};
        send_command(EL133UF1_TRES, CS_BOTH, d, sizeof(d));
    }
    {
        const uint8_t d[] = {0x0F, 0x00, 0x28, 0x2C, 0x28, 0x38};
        send_command(EL133UF1_PWR, CS0_SEL, d, sizeof(d));
    }
    {
        const uint8_t d[] = {0x07};
        send_command(EL133UF1_EN_BUF, CS0_SEL, d, sizeof(d));
    }
    {
        const uint8_t d[] = {0xD8, 0x18};
        send_command(EL133UF1_BTST_P, CS0_SEL, d, sizeof(d));
    }
    {
        const uint8_t d[] = {0x01};
        send_command(EL133UF1_BOOST_VDDP_EN, CS0_SEL, d, sizeof(d));
    }
    {
        const uint8_t d[] = {0xD8, 0x18};
        send_command(EL133UF1_BTST_N, CS0_SEL, d, sizeof(d));
    }
    {
        const uint8_t d[] = {0x01};
        send_command(EL133UF1_BUCK_BOOST_VDDN, CS0_SEL, d, sizeof(d));
    }
    {
        const uint8_t d[] = {0x02};
        send_command(EL133UF1_TFT_VCOM_POWER, CS0_SEL, d, sizeof(d));
    }

    // --- Trigger refresh (no pixel data) ---
    Serial.println("PON");
    send_command(EL133UF1_PON, CS_BOTH);
    busy_wait(200);   // mirrors busy_wait(0.2)

    Serial.println("DRF");
    {
        const uint8_t d[] = {0x00};
        send_command(EL133UF1_DRF, CS_BOTH, d, sizeof(d));
    }
    busy_wait(32000); // mirrors busy_wait(32.0)

    Serial.println("POF");
    {
        const uint8_t d[] = {0x00};
        send_command(EL133UF1_POF, CS_BOTH, d, sizeof(d));
    }
    busy_wait(200);   // mirrors busy_wait(0.2)

    Serial.println("Done.");
}

void loop() {}
