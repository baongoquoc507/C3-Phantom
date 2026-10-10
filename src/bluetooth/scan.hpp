#pragma once

#include "utils.hpp"
#include "../display_utils.h"
#include "../ui/phantom_ui.hpp"
#include "../menu.hpp"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

static std::vector<String> g_found;
static BLEScan* pBLEScan = nullptr;

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice dev) {
        String addr = dev.getAddress().toString().c_str();
        String name = dev.getName().c_str();
        int rssi    = dev.getRSSI();

        String entry = (name.length() > 0)
            ? name + " [" + addr + "] " + String(rssi) + "dB"
            : addr + " " + String(rssi) + "dB";

        if (dev.haveManufacturerData()) {
            std::string mfg = dev.getManufacturerData();
            entry += " MFG:";
            for (size_t i = 0; i < min(mfg.length(),(size_t)4); i++) {
                char buf[3]; sprintf(buf,"%02X",(uint8_t)mfg[i]);
                entry += buf;
            }
        }
        if (std::find(g_found.begin(),g_found.end(),entry)==g_found.end())
            g_found.push_back(entry);
    }
};

void BLE_Scan()
{
    g_found.clear();
    PhUI_Status("Quet Bluetooth","Dang khoi dong...","",nullptr,"CTR=Dung");

    // Khởi động BLE an toàn — chỉ init nếu chưa chạy
    if (!btStarted()) {
        BLEDevice::init("");
        delay(100);
    }

    pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);

    HaltTillRelease(BUTTON_CENTER);

    while (!ReadButton(BUTTON_CENTER)) {
        g_found.clear();
        pBLEScan->start(3, false);

        char l2[28];
        snprintf(l2, 28, "Tim duoc: %d thiet bi", (int)g_found.size());
        PhUI_Status("Quet Bluetooth", "Dang quet...", l2, nullptr, "CTR=Dung");
        pBLEScan->clearResults();
        delay(10);
    }
    HaltTillRelease(BUTTON_CENTER);

    // Deinit BLE sau khi quét xong để spam có thể dùng
    if (pBLEScan) { pBLEScan->stop(); pBLEScan = nullptr; }
    BLEDevice::deinit(false);
    delay(100);

    if (g_found.empty()) {
        PhUI_Notify("Khong tim thay\nthiet bi nao!", 1500);
        return;
    }

    // Hiển thị kết quả
    Menu* menu = new Menu();
    menu->heading_override = "Ket Qua BT (" + String(g_found.size()) + ")";
    for (auto& dev : g_found)
        menu->AddItem(MenuItem(dev, [](){}));

    bool running = true;
    menu->AddItem(MenuItem("< Quay lai", [&](){ running = false; }));
    menu->Revive(nullptr);

    while (running) {
        menu->HandleButtons();
        menu->Render();
        delay(10);
    }
    delete menu;
    g_found.clear();
}
