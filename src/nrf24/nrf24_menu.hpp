#pragma once
/*
 * ============================================================
 *  NRF24 MENU - Entry point tong hop
 * ============================================================
 *  Include file nay duy nhat tu main.cpp.
 *  No include tat ca 3 module con.
 * ============================================================
 */

// Thu tu quan trong: common truoc, sau do cac module
#include "nrf24_common.hpp"
#include "nrf24_spectrum.hpp"
#include "nrf24_jammer.hpp"
#include "nrf24_mousejack.hpp"

#include "../menu.hpp"
#include "../global.hpp"
#include "../display_utils.h"

void NRF24_Menu()
{
    Menu* m = new Menu();

    m->AddItem(MenuItem("Spectrum Analyzer",  []() { NRF24_Spectrum(); }));
    m->AddItem(MenuItem("Jammer",             []() { NRF24_Jammer(); }));
    m->AddItem(MenuItem("MouseJack",          []() { NRF24_MouseJack_Menu(); }));
    m->AddItem(MenuItem("Jammer Mode List",   []() {
        // Hien danh sach che do jammer
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.setCursor(0,0); display.print("Jammer Modes:");
        display.drawLine(0,9,127,9,WHITE);
        int y = 12;
        for (int i = 0; i < JAM_MODE_COUNT && y < 56; i++) {
            display.setCursor(0, y);
            char buf[24];
            snprintf(buf, sizeof(buf), "%d.%s(%dch)",
                     i+1, JAM_MODES[i].name, (int)JAM_MODES[i].count);
            display.print(buf);
            y += 9;
        }
        display.setCursor(0, 56); display.print("CTR=Back");
        display.display();
        HaltTillPress(BUTTON_CENTER);
        HaltTillRelease(BUTTON_CENTER);
    }));

    bool run = true;
    m->AddItem(MenuItem("[Back]", [&]() { run = false; }));
    m->Revive(nullptr);
    while (run) { m->HandleButtons(); m->Render(); delay(10); }
    delete m;
}
