#pragma once
/*
 * ============================================================
 *  NRF24 JAMMER - Adapted for C3-Phantom
 * ============================================================
 *  Port tu: Bruce firmware nrf_jammer.cpp
 *  Thay: tft/drawMainBorder/padprintln/check(NextPress)
 *  Bang: display SSD1306 + BUTTON_LEFT/CENTER/RIGHT
 *
 *  Dieu khien:
 *    LEFT        = Mode truoc
 *    RIGHT       = Mode sau
 *    CENTER      = Doi che do hop (Sequential/FHSS)
 *    Giu CENTER  = Thoat
 *
 *  Che do jam:
 *    Test | WiFi | BLE | BLE-Adv | Bluetooth
 *    USB  | Video | RC | Zigbee | Full
 * ============================================================
 */

#include "nrf24_common.hpp"
#include "../menu.hpp"

// --- Cac tap kenh jam (lay nguyen van tu Bruce) ---
static const byte JAM_TEST[]      = {50,52,54,56,58,60,62,64,66,68,70,72,74,76,78,80,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,40,42,44,46,48};
static const byte JAM_WIFI[]      = {2,7,12,17,22,27,32,37,42,47,52,57,62,67,72,77};
static const byte JAM_BLE[]       = {2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41};
static const byte JAM_BLE_ADV[]   = {37,38,39,1,2,3,25,26,27,79,80,81};
static const byte JAM_BT[]        = {2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80};
static const byte JAM_USB[]       = {32,34,36,38,40,42,44,46,48,50,52,54,56,58,60,62,64,66,68,70};
static const byte JAM_VIDEO[]     = {60,62,64,66,68,70,72,74,76,78,80,82,84,86,88,90,92,94,96,98,100,102,104,106,108,110,112,114,116,118,120,122,124};
static const byte JAM_RC[]        = {1,3,5,7,9,11,13,15,17,19,21,23,25,27,29,31,33,35,37,39};
static const byte JAM_ZIGBEE[]    = {4,5,6,9,10,11,14,15,16,19,20,21,24,25,26,29,30,31,34,35,36,39,40,41,44,45,46,49,50,51,54,55,56,59,60,61,64,65,66,69,70,71,74,75,76,79,80,81};
static const byte JAM_FULL[]      = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124};

struct JamMode {
    const char* name;
    const byte* channels;
    size_t      count;
};

static const JamMode JAM_MODES[] = {
    {"Test",        JAM_TEST,    sizeof(JAM_TEST)},
    {"WiFi",        JAM_WIFI,    sizeof(JAM_WIFI)},
    {"BLE",         JAM_BLE,     sizeof(JAM_BLE)},
    {"BLE Adv",     JAM_BLE_ADV, sizeof(JAM_BLE_ADV)},
    {"Bluetooth",   JAM_BT,      sizeof(JAM_BT)},
    {"USB",         JAM_USB,     sizeof(JAM_USB)},
    {"Video",       JAM_VIDEO,   sizeof(JAM_VIDEO)},
    {"RC",          JAM_RC,      sizeof(JAM_RC)},
    {"Zigbee",      JAM_ZIGBEE,  sizeof(JAM_ZIGBEE)},
    {"Full 2.4GHz", JAM_FULL,    sizeof(JAM_FULL)},
};
static const int JAM_MODE_COUNT = sizeof(JAM_MODES) / sizeof(JAM_MODES[0]);

static void NRF24_Jammer_DrawUI(int modeIdx, uint8_t hopMode,
                                  uint32_t hopCnt, uint8_t curCh)
{
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);

    // Tieu de
    display.setCursor(0, 0);
    display.print("NRF24 Jammer");
    display.drawLine(0, 9, 127, 9, WHITE);

    // Mode hien tai (lam noi bat)
    display.fillRect(0, 12, 128, 13, WHITE);
    display.setTextColor(BLACK);
    display.setCursor(2, 14);
    display.print(JAM_MODES[modeIdx].name);
    display.setCursor(80, 14);
    char buf[8];
    snprintf(buf, sizeof(buf), "%d/%d", modeIdx+1, JAM_MODE_COUNT);
    display.print(buf);
    display.setTextColor(WHITE);

    // Thong tin jam
    display.setCursor(0, 28);
    display.print("Hop:");
    display.print(hopMode == 0 ? "Seq" : "FHSS");
    display.setCursor(64, 28);
    display.print("CH:");
    display.print(curCh);

    display.setCursor(0, 39);
    display.print("Hops:");
    display.print(hopCnt);

    // Huong dan
    display.setCursor(0, 55);
    display.print("L/R=Mode CTR=Hop/Exit");

    display.display();
}

static void shuffleBytes(byte* arr, size_t n)
{
    for (size_t i = n-1; i > 0; i--) {
        size_t j = esp_random() % (i+1);
        byte tmp = arr[i]; arr[i] = arr[j]; arr[j] = tmp;
    }
}

void NRF24_Jammer()
{
    if (!NRF24_Begin()) return;

    // Cau hinh NRF24 cho jam
    NRFradio.setPALevel(RF24_PA_MAX);
    NRFradio.setAddressWidth(5);
    NRFradio.setPayloadSize(2);

    // Thu 2Mbps -> 1Mbps -> 250kbps
    if (!NRFradio.setDataRate(RF24_2MBPS))
        if (!NRFradio.setDataRate(RF24_1MBPS))
            NRFradio.setDataRate(RF24_250KBPS);

    NRFradio.startConstCarrier(RF24_PA_MAX, JAM_MODES[0].channels[0]);

    int    modeIdx    = 0;
    uint8_t hopMode   = 0;   // 0 = sequential, 1 = FHSS
    int    hopIdx     = 0;
    uint32_t hopCnt   = 0;
    bool   redraw     = true;
    bool   need_shuf  = true;
    byte   shuf[sizeof(JAM_FULL)] = {0};

    HaltTillRelease(BUTTON_CENTER);
    uint32_t ctr_hold = 0;

    while (true) {
        // --- Xu ly nut ---
        if (ReadButton(BUTTON_LEFT)) {
            HaltTillRelease(BUTTON_LEFT);
            modeIdx = (modeIdx - 1 + JAM_MODE_COUNT) % JAM_MODE_COUNT;
            hopIdx = 0; need_shuf = true; redraw = true;
        }
        if (ReadButton(BUTTON_RIGHT)) {
            HaltTillRelease(BUTTON_RIGHT);
            modeIdx = (modeIdx + 1) % JAM_MODE_COUNT;
            hopIdx = 0; need_shuf = true; redraw = true;
        }
        if (ReadButton(BUTTON_CENTER)) {
            // Giu 600ms = thoat, nhan nhanh = doi hop mode
            ctr_hold = millis();
            while (ReadButton(BUTTON_CENTER)) {
                if (millis() - ctr_hold > 600) {
                    HaltTillRelease(BUTTON_CENTER);
                    goto jammer_exit;
                }
                delay(20);
            }
            // Nhan nhanh = doi hop mode
            hopMode = (hopMode + 1) % 2;
            hopIdx = 0; need_shuf = true; redraw = true;
        }

        // --- Thuc hien jam ---
        size_t ch_count = JAM_MODES[modeIdx].count;

        if (hopMode == 1 && need_shuf) {
            // Tron mang kenh
            memcpy(shuf, JAM_MODES[modeIdx].channels, ch_count);
            shuffleBytes(shuf, ch_count);
            need_shuf = false;
        }

        hopIdx = (hopIdx + 1) % ch_count;
        uint8_t cur_ch = (hopMode == 0)
                         ? JAM_MODES[modeIdx].channels[hopIdx]
                         : shuf[hopIdx];

        NRFradio.setChannel(cur_ch);
        hopCnt++;

        if (redraw || hopCnt % 500 == 0) {
            NRF24_Jammer_DrawUI(modeIdx, hopMode, hopCnt, cur_ch);
            redraw = false;
        }

        delayMicroseconds(200);
    }

jammer_exit:
    NRFradio.stopConstCarrier();
    NRF24_End();

    display.clearDisplay();
    char buf[32];
    snprintf(buf, sizeof(buf), "Jammer OFF\nHops: %lu", (unsigned long)hopCnt);
    Display_PrintCentered(buf);
    display.display();
    delay(1500);
}
