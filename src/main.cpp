#include "global.hpp"
#include "menu.hpp"
#include "ui/phantom_ui.hpp"

#include "infrared/tvbgone.hpp"
#include "infrared/spammer.hpp"
#include "infrared/universal_remote.hpp"

#include "wifi/utils.hpp"
#include "wifi/scan.hpp"
#include "wifi/spam.hpp"
#include "wifi/deauth_bruce.hpp"

#include "bluetooth/spam.hpp"
#include "bluetooth/scan.hpp"

#include "rf/tesla.hpp"
#include "rf/jammer.hpp"
#include "rf/identifier.hpp"
#include "rf/scan.hpp"
#include "rf/send.hpp"

#include "nrf24/nrf24_menu.hpp"

#include "headless.h"
#include "settings.h"

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include <LittleFS.h>

// ============================================================
//  KHAI BAO MENU
// ============================================================
Menu menu;

Menu menu_wifi;
Menu menu_wifi_deauth;

Menu menu_bluetooth;

Menu menu_hongngoi;
Menu menu_tvbgone;
Menu menu_dieukien_xa;

Menu menu_radio;
Menu menu_radio_gui;

Menu menu_caidat;

Menu* active_menu = nullptr;

// ============================================================
//  SETUP
// ============================================================
void setup()
{
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
    Serial.begin(115200);

    bool display_ok = display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    if (!display_ok)
        Serial.println("[UI] SSD1306 khong tim thay, chay headless...");

    // Pin setup
    pinMode(IR_TX,         OUTPUT);
    pinMode(BUTTON_LEFT,   INPUT_PULLUP);
    pinMode(BUTTON_CENTER, INPUT_PULLUP);
    pinMode(BUTTON_RIGHT,  INPUT_PULLUP);

    digitalWrite(IR_TX, LOW);
    gpio_set_drive_capability(gpio_num_t(IR_TX), GPIO_DRIVE_CAP_3);

    // Filesystem
    if (!LittleFS.begin()) {
        display.clearDisplay();
        Display_PrintCentered("Loi he thong\ntep tin!");
        display.display();
        delay(1000);
        while (true) {
            display.clearDisplay();
            Display_PrintCentered("Giu CTR\nde dinh dang");
            display.display();
            static unsigned long t0 = 0;
            if (ReadButton(BUTTON_CENTER)) {
                if (!t0) t0 = millis();
                if (millis() - t0 > 1000) { LittleFS.begin(true); ESP.restart(); }
            } else t0 = 0;
            delay(100);
        }
    }

    if (!display_ok) { Headless_Setup(); return; }

    auto cfg = ReadConfig("/config.cfg");
    if (cfg["headless"] == "1") { Headless_Setup(); return; }

    // ── Splash screen ──────────────────────────────────────
    PhUI_Splash(2000);

    // ============================================================
    //  MENU WIFI
    // ============================================================
    menu_wifi.heading_override = "WiFi";
    menu_wifi.AddItem(MenuItem("Quet Mang",      WiFi_Scan));
    menu_wifi.AddItem(MenuItem("Spam Beacon",    WiFi_BeaconSpam));
    menu_wifi.AddItem(MenuItem("Ngat Ket Noi",   &menu_wifi_deauth));

    menu_wifi_deauth.heading_override = "Ngat Ket Noi";
    menu_wifi_deauth.AddItem(MenuItem("Tu Quet",        []() { Bruce_DeauthFromScan();  }));
    menu_wifi_deauth.AddItem(MenuItem("Chon Thiet Bi",  []() { Bruce_TargetDeauth();    }));
    menu_wifi_deauth.AddItem(MenuItem("Tan Cong Tat Ca",[]() { Bruce_DeauthFlood();     }));
    menu_wifi_deauth.AddItem(MenuItem("Theo Kenh",      []() { Bruce_DeauthByChannel(); }));

    // ============================================================
    //  MENU BLUETOOTH
    // ============================================================
    menu_bluetooth.heading_override = "Bluetooth";
    menu_bluetooth.AddItem(MenuItem("Quet Thiet Bi",  BLE_Scan));
    menu_bluetooth.AddItem(MenuItem("Spam Ket Noi",   BLE_Spam));

    // ============================================================
    //  MENU HONG NGOAI
    // ============================================================
    menu_hongngoi.heading_override = "Hong Ngoai";
    menu_hongngoi.AddItem(MenuItem("Tat TV",          &menu_tvbgone));
    menu_hongngoi.AddItem(MenuItem("Spam Hong Ngoai", Ir_Spammer));
    menu_hongngoi.AddItem(MenuItem("Dieu Khien Xa",   &menu_dieukien_xa));

    menu_tvbgone.heading_override = "Tat TV";
    menu_tvbgone.AddItem(MenuItem("Chau Au (EU)", TvbGone_Callback_EU));
    menu_tvbgone.AddItem(MenuItem("Bac My (NA)", TvbGone_Callback_NA));

    menu_dieukien_xa.heading_override = "Dieu Khien Xa";
    menu_dieukien_xa.AddItem(MenuItem("Ti vi",      [](){ IR_UniversalRemote(REMOTE_TYPE_TV);         }));
    menu_dieukien_xa.AddItem(MenuItem("May Chieu",  [](){ IR_UniversalRemote(REMOTE_TYPE_PROJECTOR);  }));

    // ============================================================
    //  MENU RADIO (CC1101)
    // ============================================================
    menu_radio.heading_override = "Radio (Sub-GHz)";
    menu_radio.AddItem(MenuItem("Quet RF",         RF_Scan));
    menu_radio.AddItem(MenuItem("Gui Tin Hieu",    RF_Send));
    menu_radio.AddItem(MenuItem("Gay Nhieu RF",    RF_Jammer));
    menu_radio.AddItem(MenuItem("Nhan Dang GD",    RF_Identifier));
    menu_radio.AddItem(MenuItem("Tesla Sac",       RF_TeslaChargePort));

    // ============================================================
    //  MENU CAI DAT
    // ============================================================
    menu_caidat.heading_override = "Cai Dat";
    menu_caidat.AddItem(MenuItem("Che Do An", []() {
        auto cfg = ReadConfig("/config.cfg");
        bool cur  = (cfg["headless"] == "1");
        cfg["headless"] = cur ? "0" : "1";
        WriteConfig("/config.cfg", cfg);
        PhUI_Notify(cur ? "Da tat che do an" : "Da bat che do an", 800);
        ESP.restart();
    }));
    menu_caidat.AddItem(MenuItem("Khoi Dong Lai", []() {
        if (PhUI_Confirm("Xac Nhan", "Khoi dong lai?")) ESP.restart();
    }));

    // ============================================================
    //  MENU CHINH
    // ============================================================
    menu.AddItem(MenuItem("WiFi",          &menu_wifi));
    menu.AddItem(MenuItem("Bluetooth",     &menu_bluetooth));
    menu.AddItem(MenuItem("Hong Ngoai",    &menu_hongngoi));
    menu.AddItem(MenuItem("Radio",         &menu_radio));
    menu.AddItem(MenuItem("NRF24",         []() { NRF24_Menu(); }));
    menu.AddItem(MenuItem("Cai Dat",       &menu_caidat));

    menu.Revive(nullptr);
    active_menu = &menu;
}

// ============================================================
//  LOOP
// ============================================================
void loop()
{
    digitalWrite(IR_TX, LOW);
    if (!active_menu) return;

    Menu* next = active_menu->HandleButtons();
    if (next) active_menu = next;
    active_menu->Render();
}

