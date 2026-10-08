#pragma once
/*
 * NRF24 MENU — Viet hoa + giao dien Phantom UI
 */
#include "nrf24_common.hpp"
#include "nrf24_spectrum.hpp"
#include "nrf24_jammer.hpp"
#include "nrf24_mousejack.hpp"
#include "../menu.hpp"
#include "../ui/phantom_ui.hpp"

void NRF24_Menu()
{
    Menu* m = new Menu();
    m->heading_override = "NRF24 2.4GHz";

    m->AddItem(MenuItem("Phan Tich Pho",   []() { NRF24_Spectrum();       }));
    m->AddItem(MenuItem("Gay Nhieu 2.4G",  []() { NRF24_Jammer();         }));
    m->AddItem(MenuItem("MouseJack",       []() { NRF24_MouseJack_Menu(); }));
    m->AddItem(MenuItem("Danh Sach Che Do",[]() {
        display.clearDisplay();
        PhUI_Header("Che Do Gay Nhieu");
        display.setTextSize(1);
        display.setTextColor(WHITE);
        int y = 13;
        for (int i = 0; i < JAM_MODE_COUNT && y < 52; i++) {
            char buf[24];
            snprintf(buf, sizeof(buf), "%d. %s (%dkH)",
                     i+1, JAM_MODES[i].name, (int)JAM_MODES[i].count);
            display.setCursor(3, y); display.print(buf);
            y += 9;
        }
        PhUI_Footer(nullptr, "CTR=Quay lai", nullptr);
        display.display();
        HaltTillPress(BUTTON_CENTER);
        HaltTillRelease(BUTTON_CENTER);
    }));

    bool run = true;
    m->AddItem(MenuItem("< Quay lai", [&](){ run = false; }));
    m->Revive(nullptr);
    while (run) { m->HandleButtons(); m->Render(); delay(10); }
    delete m;
}
