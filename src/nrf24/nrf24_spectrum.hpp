#pragma once
/*
 * ============================================================
 *  NRF24 SPECTRUM ANALYZER - Adapted for SSD1306 128x64
 * ============================================================
 *  Port tu: Bruce firmware nrf_spectrum.cpp
 *  Thay tft (TFT) bang display (SSD1306)
 *
 *  Hien thi:
 *    - 80 kenh (0-79) tuong ung 2.400-2.479 GHz
 *    - Moi kenh = 1 pixel rong, bar cao theo muc tin hieu
 *    - Dong cuoi: nhan 5: chu thich GHz
 *    - CENTER de thoat
 * ============================================================
 */

#include "nrf24_common.hpp"

#define SPEC_CHANNELS  80
#define SPEC_BAR_W     1              // 80 kenh * 1px = 80px (vua man hinh 128px)
#define SPEC_BAR_MAXH  40             // Chieu cao toi da bar (pixel)
#define SPEC_BAR_BASEY 52             // Y co so (duoi cung cua bar zone)

static uint8_t spec_level[SPEC_CHANNELS] = {0};

static void NRF24_Spectrum_DrawBars()
{
    // Xoa vung bar
    display.fillRect(0, SPEC_BAR_BASEY - SPEC_BAR_MAXH, 128, SPEC_BAR_MAXH + 2, BLACK);

    for (int i = 0; i < SPEC_CHANNELS; i++) {
        int h = (spec_level[i] * SPEC_BAR_MAXH) / 255;
        if (h < 1 && spec_level[i] > 0) h = 1;
        int x = i * SPEC_BAR_W;
        int y = SPEC_BAR_BASEY - h;
        // Kenh chan mau sang, le mau mo
        display.drawFastVLine(x, y, h, (i % 2 == 0) ? WHITE : INVERSE);
    }

    // Gach ngang nen
    display.drawFastHLine(0, SPEC_BAR_BASEY, 128, WHITE);
}

static void NRF24_Spectrum_DrawLabels()
{
    // Xoa dong nhan
    display.fillRect(0, 54, 128, 10, BLACK);
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0,  55); display.print("2.40");
    display.setCursor(46, 55); display.print("2.44");
    display.setCursor(94, 55); display.print("2.48");
}

static void NRF24_Spectrum_ScanOnce()
{
    static const uint8_t noise_addr[][2] = {
        {0x55,0x55},{0xAA,0xAA},{0xA0,0xAA},
        {0xAB,0xAA},{0xAC,0xAA},{0xAD,0xAA}
    };

    // Tat CE truoc khi set kenh
    digitalWrite(NRF24_CE_PIN, LOW);

    for (int i = 0; i < SPEC_CHANNELS; i++) {
        NRFradio.setChannel(i);
        NRFradio.startListening();
        delayMicroseconds(130);
        NRFradio.stopListening();

        // testRPD() = Received Power Detector (>-64dBm)
        uint8_t rpd = NRFradio.testRPD() ? 200 : 0;
        // EMA smooth: level = (level*3 + rpd) / 4
        spec_level[i] = (spec_level[i] * 3 + rpd) >> 2;
    }
}

void NRF24_Spectrum()
{
    if (!NRF24_Begin()) return;

    NRFradio.setAutoAck(false);
    NRFradio.disableCRC();
    NRFradio.setAddressWidth(2);
    NRFradio.setDataRate(RF24_1MBPS);

    // Set 6 ong nghe nhieu
    const uint8_t noise_addr[][2] = {
        {0x55,0x55},{0xAA,0xAA},{0xA0,0xAA},
        {0xAB,0xAA},{0xAC,0xAA},{0xAD,0xAA}
    };
    for (uint8_t p = 0; p < 6; p++)
        NRFradio.openReadingPipe(p, noise_addr[p]);

    // Ve khung co dinh
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(25, 0);
    display.print("Phan Tich Pho 2.4G");
    display.drawLine(0, 9, 127, 9, WHITE);
    NRF24_Spectrum_DrawLabels();
    display.display();

    HaltTillRelease(BUTTON_CENTER);
    uint32_t frame = 0;

    while (!ReadButton(BUTTON_CENTER)) {
        NRF24_Spectrum_ScanOnce();
        NRF24_Spectrum_DrawBars();

        // Cap nhat tieu de moi 20 frame
        if (++frame % 20 == 0) {
            display.fillRect(0, 0, 128, 9, BLACK);
            display.setTextColor(WHITE);
            display.setCursor(0, 0);
            display.print("Phan Tich Pho");
            display.setCursor(70, 0);
            char buf[12];
            snprintf(buf, sizeof(buf), "f=%lu kHz", frame);
            display.print(buf);
        }

        display.display();
        delay(10);
    }

    HaltTillRelease(BUTTON_CENTER);
    NRF24_End();
}
