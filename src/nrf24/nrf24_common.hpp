#pragma once
/*
 * ============================================================
 *  NRF24 COMMON - Adapted for C3-Phantom
 * ============================================================
 *  Thay the Bruce's: bruceConfig, bruceConfigPins, bus_HAL
 *  Bang: SPI co dinh, RF24 library
 *
 *  Pin assignment:
 *    CE   = GPIO 20  (Chip Enable)
 *    CSN  = GPIO 0   (SPI Chip Select)
 *    SCK  = GPIO 4   (SPI Clock)
 *    MOSI = GPIO 6   (SPI MOSI)
 *    MISO = GPIO 5   (SPI MISO)
 * ============================================================
 */

#include <Arduino.h>
#include <SPI.h>

// ir_codes.hpp dinh nghia: #define NOP __asm__ __volatile__ ("nop")
// RF24/nRF24L01.h dinh nghia: constexpr uint8_t NOP = 0xFF;
// → Phai undef NOP truoc khi include RF24 de tranh xung dot
#ifdef NOP
#undef NOP
#endif
#include <RF24.h>

#include "../global.hpp"
#include "../display_utils.h"

// --- Pin definitions ---
#define NRF24_CE_PIN    20  // Chip Enable
#define NRF24_CSN_PIN   0   // SPI Chip Select
#define NRF24_SCK_PIN   4   // SPI Clock
#define NRF24_MOSI_PIN  6   // SPI MOSI
#define NRF24_MISO_PIN  5   // SPI MISO

// --- Shared SPI bus for NRF24 ---
static SPIClass nrf24_spi(FSPI);
static RF24     NRFradio(NRF24_CE_PIN, NRF24_CSN_PIN);
static bool     nrf24_initialized = false;

// --- Helper: Hien tieu de tren OLED ---
static void NRF24_DrawTitle(const char* title, const char* sub = nullptr)
{
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.print(title);
    display.drawLine(0, 9, 127, 9, WHITE);
    if (sub) { display.setCursor(0, 13); display.print(sub); }
    display.display();
}

// --- Helper: In dong trang thai ---
static void NRF24_Status(const char* l1, const char* l2 = nullptr,
                          const char* l3 = nullptr, const char* hint = nullptr)
{
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    if (l1) display.print(l1);
    display.drawLine(0, 9, 127, 9, WHITE);
    if (l2) { display.setCursor(0, 13); display.print(l2); }
    if (l3) { display.setCursor(0, 25); display.print(l3); }
    if (hint) {
        display.setCursor(0, 55);
        display.print(hint);
    } else {
        display.setCursor(0, 55);
        display.print("CTR=Stop");
    }
    display.display();
}

// --- Khoi tao NRF24 qua FSPI ---
static bool NRF24_Begin()
{
    if (nrf24_initialized) return true;

    nrf24_spi.begin(NRF24_SCK_PIN, NRF24_MISO_PIN, NRF24_MOSI_PIN, NRF24_CSN_PIN);
    // SCK=4, MISO=5, MOSI=6, CSN=0, CE=20
    delay(10);

    if (!NRFradio.begin(&nrf24_spi)) {
        display.clearDisplay();
        Display_PrintCentered("NRF24\nnot found!\nCheck wiring");
        display.display();
        delay(2500);
        return false;
    }

    nrf24_initialized = true;
    Serial.println("[NRF24] Initialized OK");
    return true;
}

// --- Tat NRF24 ---
static void NRF24_End()
{
    if (!nrf24_initialized) return;
    NRFradio.stopListening();
    NRFradio.powerDown();
    nrf24_spi.end();
    nrf24_initialized = false;
    delay(100);
    // Giai phong CS pin
    pinMode(NRF24_CSN_PIN, INPUT);
    pinMode(NRF24_CE_PIN,  INPUT);
}
