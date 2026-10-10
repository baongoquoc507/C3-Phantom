#pragma once
/*
 * ============================================================
 *  PHANTOM UI  —  Giao dien kieu Flipper Zero cho SSD1306
 * ============================================================
 *  Layout 128x64:
 *
 *  ┌──────────────────────────────┐ y=0
 *  │  ◄  TIEU DE            [*]  │       Header  (10px)
 *  ├──────────────────────────────┤ y=10
 *  │                              │       Content (43px)
 *  │   (menu / status / etc.)     │
 *  │                              │
 *  ├──────────────────────────────┤ y=53
 *  │ [TRAI]   [GIUA]   [PHAI]    │       Footer  (11px)
 *  └──────────────────────────────┘ y=63
 * ============================================================
 */

#include "../global.hpp"
#include <vector>

// ──────────────────────────────────────────────────────────
//  HANG SO LAYOUT
// ──────────────────────────────────────────────────────────
#define PH_W          128
#define PH_H           64
#define PH_HDR_H       11   // chieu cao header
#define PH_FTR_Y       54   // y bat dau footer
#define PH_FTR_H       10
#define PH_CONTENT_Y   12   // y bat dau vung noi dung
#define PH_CONTENT_H   41   // chieu cao vung noi dung
#define PH_ITEM_H      11   // chieu cao moi dong menu
#define PH_VISIBLE      3   // so dong hien thi cung luc

// ──────────────────────────────────────────────────────────
//  VE HEADER
//  title   : ten hien thi giua
//  has_back: hien mui ten < ben trai
//  has_next: hien mui ten > ben phai
// ──────────────────────────────────────────────────────────
static void PhUI_Header(const char* title,
                         bool has_back = true, bool has_fwd = false)
{
    // Nen header
    display.fillRect(0, 0, PH_W, PH_HDR_H, WHITE);
    display.setTextColor(BLACK);
    display.setTextSize(1);

    // Mui ten trai
    if (has_back) {
        display.setCursor(2, 2);
        display.print("\x1B"); // ← (custom: dung '<')
        display.setCursor(2, 2);
        display.print("<");
    }

    // Tieu de can giua
    int tw = strlen(title) * 6;
    int tx = (PH_W - tw) / 2;
    if (tx < 10) tx = 10;
    display.setCursor(tx, 2);
    display.print(title);

    // Mui ten phai hoac dau cham
    if (has_fwd) {
        display.setCursor(PH_W - 8, 2);
        display.print(">");
    } else {
        // Hien dau bao hieu la menu chinh (khong co back)
        if (!has_back) {
            display.setCursor(PH_W - 14, 2);
            display.print("--");
        }
    }

    display.setTextColor(WHITE);
    // Duong ke ngang
    display.drawFastHLine(0, PH_HDR_H, PH_W, WHITE);
}

// ──────────────────────────────────────────────────────────
//  VE FOOTER (goi y nut bam)
// ──────────────────────────────────────────────────────────
static void PhUI_Footer(const char* left = nullptr,
                          const char* mid  = nullptr,
                          const char* right = nullptr)
{
    display.drawFastHLine(0, PH_FTR_Y - 1, PH_W, WHITE);

    display.setTextSize(1);
    display.setTextColor(WHITE);

    if (left) {
        display.setCursor(2, PH_FTR_Y + 1);
        display.print(left);
    }
    if (mid) {
        int tw = strlen(mid) * 6;
        display.setCursor((PH_W - tw) / 2, PH_FTR_Y + 1);
        display.print(mid);
    }
    if (right) {
        int tw = strlen(right) * 6;
        display.setCursor(PH_W - tw - 2, PH_FTR_Y + 1);
        display.print(right);
    }
}

// ──────────────────────────────────────────────────────────
//  MAN HINH TRANG THAI  (header + toi da 3 dong + footer)
// ──────────────────────────────────────────────────────────
static void PhUI_Status(const char* title,
                          const char* l1    = nullptr,
                          const char* l2    = nullptr,
                          const char* l3    = nullptr,
                          const char* hint  = "CTR=Dung")
{
    display.clearDisplay();
    PhUI_Header(title);

    display.setTextSize(1);
    display.setTextColor(WHITE);

    if (l1) { display.setCursor(3, PH_CONTENT_Y + 2);  display.print(l1); }
    if (l2) { display.setCursor(3, PH_CONTENT_Y + 13); display.print(l2); }
    if (l3) { display.setCursor(3, PH_CONTENT_Y + 24); display.print(l3); }

    if (hint) PhUI_Footer(nullptr, hint, nullptr);
    display.display();
}

// ──────────────────────────────────────────────────────────
//  CAP NHAT DONG DEM (chi lam moi vung noi dung, ko clear het)
// ──────────────────────────────────────────────────────────
static void PhUI_UpdateLine(int lineIdx, const char* text)
{
    // lineIdx: 0,1,2
    int y = PH_CONTENT_Y + 2 + lineIdx * 11;
    display.fillRect(0, y, PH_W, 10, BLACK);
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(3, y);
    display.print(text);
    display.display();
}

// ──────────────────────────────────────────────────────────
//  THONG BAO POPUP  (hien 1 dong canh bao giua man hinh)
// ──────────────────────────────────────────────────────────
static void PhUI_Notify(const char* msg, uint32_t ms = 1500)
{
    display.clearDisplay();

    // Hop canh bao can giua
    int bw = 120, bh = 28;
    int bx = (PH_W - bw) / 2, by = (PH_H - bh) / 2;
    display.fillRoundRect(bx - 1, by - 1, bw + 2, bh + 2, 3, WHITE);
    display.fillRoundRect(bx, by, bw, bh, 3, BLACK);
    display.drawRoundRect(bx, by, bw, bh, 3, WHITE);

    display.setTextSize(1);
    display.setTextColor(WHITE);
    int tw = strlen(msg) * 6;
    if (tw > 116) tw = 116;
    display.setCursor((PH_W - tw) / 2, by + (bh - 8) / 2);
    // In toi da 19 ky tu
    char buf[20]; strncpy(buf, msg, 19); buf[19] = 0;
    display.print(buf);

    display.display();
    delay(ms);
}

// ──────────────────────────────────────────────────────────
//  HOP XAC NHAN  →  CTR=Co | L=Khong
//  Tra ve true neu nguoi dung bam CENTER
// ──────────────────────────────────────────────────────────
static bool PhUI_Confirm(const char* title, const char* question)
{
    display.clearDisplay();
    PhUI_Header(title, false, false);

    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(3, PH_CONTENT_Y + 5);
    display.print(question);

    PhUI_Footer("Khong", nullptr, "Co");
    display.display();

    HaltTillRelease(BUTTON_CENTER);
    HaltTillRelease(BUTTON_LEFT);

    while (true) {
        if (ReadButton(BUTTON_CENTER)) { HaltTillRelease(BUTTON_CENTER); return true;  }
        if (ReadButton(BUTTON_LEFT))   { HaltTillRelease(BUTTON_LEFT);   return false; }
        delay(20);
    }
}

// ──────────────────────────────────────────────────────────
//  THANH TIEN TRINH
// ──────────────────────────────────────────────────────────
static void PhUI_Progress(const char* title, int pct,
                            const char* msg = nullptr)
{
    display.clearDisplay();
    PhUI_Header(title);

    // Thanh progress
    int bx = 4, by = PH_CONTENT_Y + 8, bw = PH_W - 8, bh = 10;
    display.drawRoundRect(bx, by, bw, bh, 2, WHITE);
    int fill = (pct * (bw - 2)) / 100;
    if (fill > 0)
        display.fillRoundRect(bx + 1, by + 1, fill, bh - 2, 2, WHITE);

    // Phan tram
    char pbuf[8]; snprintf(pbuf, sizeof(pbuf), "%d%%", pct);
    display.setTextSize(1);
    display.setTextColor(WHITE);
    int pw = strlen(pbuf) * 6;
    display.setCursor((PH_W - pw) / 2, by + bh + 3);
    display.print(pbuf);

    if (msg) {
        display.setCursor(3, by + bh + 15);
        display.print(msg);
    }

    display.display();
}

// ──────────────────────────────────────────────────────────
//  MENU CHON (scroll list) — thay cho Bruce_SelectFromList
//  Tra ve index duoc chon, -1 neu nguoi dung chon "Quay lai"
// ──────────────────────────────────────────────────────────
static int PhUI_Select(const char* title,
                         const std::vector<String>& items,
                         const char* back_label = "Quay lai")
{
    std::vector<String> list = items;
    list.push_back(String(back_label));
    int n = (int)list.size();
    int idx = 0, off = 0;

    HaltTillRelease(BUTTON_LEFT);
    HaltTillRelease(BUTTON_CENTER);
    HaltTillRelease(BUTTON_RIGHT);
    delay(80);

    while (true) {
        display.clearDisplay();
        PhUI_Header(title);

        // Ve PH_VISIBLE dong menu
        for (int i = 0; i < PH_VISIBLE; i++) {
            int li = off + i;
            if (li >= n) break;

            int iy = PH_CONTENT_Y + i * PH_ITEM_H;
            bool sel = (li == idx);

            if (sel) {
                display.fillRect(0, iy, PH_W, PH_ITEM_H, WHITE);
                display.setTextColor(BLACK);
                display.setCursor(3, iy + 2);
                display.print(">");
            } else {
                display.setTextColor(WHITE);
                display.setCursor(3, iy + 2);
            }

            // In tieu de (cat ngan neu qua dai)
            String lb = list[li];
            if (lb.length() > 17) lb = lb.substring(0, 16) + "~";
            display.setCursor(12, iy + 2);
            display.print(lb);
            display.setTextColor(WHITE);
        }

        // Mui ten scroll
        if (off > 0) {
            display.setCursor(PH_W - 8, PH_CONTENT_Y);
            display.print("^");
        }
        if (off + PH_VISIBLE < n) {
            display.setCursor(PH_W - 8, PH_CONTENT_Y + (PH_VISIBLE - 1) * PH_ITEM_H);
            display.print("v");
        }

        PhUI_Footer("Len", "Chon", "Xuong");
        display.display();

        // Xu ly nut
        while (true) {
            if (ReadButton(BUTTON_LEFT)) {
                HaltTillRelease(BUTTON_LEFT);
                if (idx > 0) {
                    idx--;
                    if (idx < off) off = idx;
                }
                break;
            }
            if (ReadButton(BUTTON_RIGHT)) {
                HaltTillRelease(BUTTON_RIGHT);
                if (idx < n - 1) {
                    idx++;
                    if (idx >= off + PH_VISIBLE) off = idx - PH_VISIBLE + 1;
                }
                break;
            }
            if (ReadButton(BUTTON_CENTER)) {
                HaltTillRelease(BUTTON_CENTER);
                // Muc cuoi = Quay lai
                if (idx == n - 1) return -1;
                return idx;
            }
            delay(25);
        }
    }
}

// ──────────────────────────────────────────────────────────
//  MAN HINH KHOI DONG  (splash screen)
// ──────────────────────────────────────────────────────────
static void PhUI_Splash(uint32_t ms = 2000)
{
    display.clearDisplay();

    // Khung ngoai
    display.drawRoundRect(1, 1, PH_W - 2, PH_H - 2, 4, WHITE);

    // Ten project
    // setTextSize(2): moi ky tu rong 12px, "C3-Phantom" = 10 ky tu = 120px
    // Man hinh 128px → cursor x = (128-120)/2 = 4 → vua khit
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(4, 10);
    display.print("C3-Phantom");

    // Dong chu nho
    display.setTextSize(1);
    display.setCursor(22, 34);
    display.print("QUOC BAO DIY");

    // Version
    display.setCursor(40, 48);
    display.print("v7.0  ESP32-C3");

    display.display();
    delay(ms);
}

// ──────────────────────────────────────────────────────────
//  MAN HINH KET QUA TAN CONG
// ──────────────────────────────────────────────────────────
static void PhUI_Result(const char* title,
                          uint32_t frames_or_count,
                          const char* unit = "Frames",
                          bool success = true)
{
    display.clearDisplay();
    PhUI_Header(title, false, false);

    display.setTextSize(1);
    display.setTextColor(WHITE);

    display.setCursor(3, PH_CONTENT_Y + 3);
    display.print(success ? "Hoan thanh!" : "That bai!");

    char buf[32];
    snprintf(buf, sizeof(buf), "%s: %lu", unit, (unsigned long)frames_or_count);
    display.setCursor(3, PH_CONTENT_Y + 16);
    display.print(buf);

    PhUI_Footer(nullptr, "CTR=Tiep", nullptr);
    display.display();

    HaltTillRelease(BUTTON_CENTER);
    HaltTillPress(BUTTON_CENTER);
    HaltTillRelease(BUTTON_CENTER);
}

