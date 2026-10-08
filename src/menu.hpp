#pragma once
/*
 * ============================================================
 *  MENU.HPP  —  Giao dien kieu Flipper Zero
 * ============================================================
 *  Layout:
 *    Header  (11px) : nen trang, ten menu, < >
 *    Content (42px) : 3 dong menu (11px/dong) + scroll
 *    Footer  (11px) : goi y nut bam
 * ============================================================
 */

#include "global.hpp"
#include <vector>

class Menu;

class MenuItem {
private:
    String title;
    std::function<void()> callback;
    Menu*  submenu  = nullptr;
    bool   is_exit  = false;

public:
    MenuItem(String _title, std::function<void()> _cb = nullptr)
        : title(_title), callback(_cb) {}

    MenuItem(String _title, Menu* _sub)
        : title(_title), submenu(_sub) {}

    static MenuItem ExitItem() {
        MenuItem m("< Quay lai");
        m.is_exit = true;
        return m;
    }

    void RunCallback(bool* exit_menu = nullptr) {
        if (exit_menu) *exit_menu = true;
        if (callback) callback();
    }

    const String& GetTitle()   const { return title;   }
    Menu*         GetSubmenu() const { return submenu;  }
    bool          IsExit()     const { return is_exit;  }
};

class Menu {
private:
    // --- Du lieu ---
    std::vector<MenuItem> items;
    int   sel     = 0;   // muc dang chon
    int   off     = 0;   // scroll offset
    Menu* parent  = nullptr;
    bool  do_exit = false;

    // --- Animation ---
    float sel_y       = 0;
    float tgt_sel_y   = 0;
    bool  entering    = false;
    float enter_prog  = 0;
    unsigned long anim_start = 0;

    // --- Text scroll ---
    int   txt_off   = 0;
    unsigned long last_txt = 0;

    // --- Constants ---
    static const int HDR_H    = 11;
    static const int ITEM_H   = 11;
    static const int VISIBLE  = 4;   // 44px / 11px = 4 dong
    static const int FTR_Y    = 54;

    void DrawHeader()
    {
        // Nen header trang
        display.fillRect(0, 0, 128, HDR_H, WHITE);
        display.setTextColor(BLACK);
        display.setTextSize(1);

        // Mui ten trai neu co parent
        if (parent) {
            display.setCursor(2, 2);
            display.print("<");
        }

        // Ten menu hien tai (dong thu nhat trong breadcrumb)
        // Su dung tieu de cua muc cha dang chon → hien muc hien tai
        // Tam thoi hien "C3-Phantom"
        const char* heading = parent ? "Menu" : "C3-Phantom";
        // Tim heading phu hop: ten submenu duoc chon tu parent
        // (don gian: hien ten app o root, "Menu" o cac cap con)
        int tw = strlen(heading) * 6;
        display.setCursor((128 - tw) / 2, 2);
        display.print(heading);

        display.setTextColor(WHITE);
        display.drawFastHLine(0, HDR_H, 128, WHITE);
    }

    void DrawFooter()
    {
        display.drawFastHLine(0, FTR_Y - 1, 128, WHITE);
        display.setTextSize(1);
        display.setTextColor(WHITE);

        if (parent) {
            display.setCursor(2, FTR_Y + 1);
            display.print("Len");
            int tw = strlen("Chon") * 6;
            display.setCursor((128 - tw) / 2, FTR_Y + 1);
            display.print("Chon");
            display.setCursor(128 - 30, FTR_Y + 1);
            display.print("Xuong");
        } else {
            display.setCursor(2, FTR_Y + 1);
            display.print("Len");
            int tw = strlen("OK") * 6;
            display.setCursor((128 - tw) / 2, FTR_Y + 1);
            display.print("OK");
            display.setCursor(128 - 30, FTR_Y + 1);
            display.print("Xuong");
        }
    }

public:
    bool is_rendering = false;
    // Cho phep menu con set tieu de hien trong header
    String heading_override = "";

    void AddItem(const MenuItem& item) { items.push_back(item); }

    void Revive(Menu* _parent = nullptr, bool = false, bool = false)
    {
        is_rendering = true;
        do_exit  = false;
        sel      = 0;
        off      = 0;
        txt_off  = 0;
        last_txt = millis();
        sel_y    = 0; tgt_sel_y = 0;
        if (_parent) parent = _parent;

        // Them nut Quay lai neu co parent va chua co
        if (parent) {
            bool has_exit = false;
            for (auto& it : items)
                if (it.IsExit()) { has_exit = true; break; }
            if (!has_exit)
                items.push_back(MenuItem::ExitItem());
        }

        entering   = true;
        enter_prog = 0;
        anim_start = millis();
    }

    void MoveUp() {
        if (sel > 0) {
            sel--;
            txt_off = 0;
            if (sel < off) off = sel;
        }
    }

    void MoveDown() {
        if (sel < (int)items.size() - 1) {
            sel++;
            txt_off = 0;
            if (sel >= off + VISIBLE) off = sel - VISIBLE + 1;
        }
    }

    Menu* Select(bool* exit_flag = nullptr)
    {
        if (items.empty()) return nullptr;

        if (items[sel].IsExit()) {
            if (parent) {
                parent->Revive(nullptr, false, true);
                return parent;
            }
            do_exit = true;
            return nullptr;
        }

        Menu* sub = items[sel].GetSubmenu();
        if (sub) {
            // Truyen ten muc hien tai vao submenu de hien thi header
            sub->heading_override = items[sel].GetTitle();
            sub->Revive(this, true, false);
            return sub;
        }

        items[sel].RunCallback(exit_flag);
        return nullptr;
    }

    Menu* HandleButtons()
    {
        if (entering) return nullptr;

        if (ReadButtonWait(BUTTON_LEFT))
            MoveUp();
        else if (ReadButtonWait(BUTTON_RIGHT))
            MoveDown();
        else if (ReadButtonWait(BUTTON_CENTER)) {
            HaltTillRelease(BUTTON_CENTER);
            return Select();
        }
        return nullptr;
    }

    Menu* GetParent() { return parent; }

    void Render(bool skip_display = false)
    {
        if (!is_rendering) return;
        if (do_exit) { is_rendering = false; return; }

        unsigned long now = millis();

        // Hieu ung vao
        if (entering) {
            float elapsed = (float)(now - anim_start);
            enter_prog = min(1.0f, elapsed / 180.0f);
            if (enter_prog >= 1.0f) entering = false;
        }

        // Noi suy vi tri highlight
        tgt_sel_y = (float)((sel - off) * ITEM_H) + HDR_H + 1;
        sel_y += (tgt_sel_y - sel_y) * 0.35f;
        if (fabsf(tgt_sel_y - sel_y) < 0.6f) sel_y = tgt_sel_y;

        display.clearDisplay();

        // ── Header ──────────────────────────────────────────
        display.fillRect(0, 0, 128, HDR_H, WHITE);
        display.setTextColor(BLACK);
        display.setTextSize(1);

        // Tieu de: neu co override thi dung, nguoc lai dung "C3-Phantom"
        const char* hd = heading_override.length() > 0
                         ? heading_override.c_str()
                         : (parent ? "C3-Phantom" : "C3-Phantom");

        if (parent) { display.setCursor(2, 2); display.print("<"); }

        int htw = strlen(hd) * 6;
        if (htw > 108) htw = 108;
        display.setCursor((128 - htw) / 2, 2);
        // In toi da 18 ky tu
        char hbuf[19]; strncpy(hbuf, hd, 18); hbuf[18] = 0;
        display.print(hbuf);

        display.setTextColor(WHITE);
        display.drawFastHLine(0, HDR_H, 128, WHITE);

        // ── Highlight bar (animate) ──────────────────────────
        int slide = (int)((1.0f - enter_prog) * 20.0f);
        display.fillRect(slide, (int)sel_y, 128 - slide, ITEM_H, WHITE);

        // ── Cac dong menu ───────────────────────────────────
        display.setTextWrap(false);
        bool show_scroll = (int)items.size() > VISIBLE;

        for (int i = 0; i < VISIBLE; i++) {
            int li = off + i;
            if (li >= (int)items.size()) break;

            int iy = HDR_H + 1 + i * ITEM_H;
            bool is_sel = (li == sel);

            if (is_sel) {
                display.setTextColor(BLACK);
                // Mui ten chon
                display.setCursor(2, iy + 2);
                display.print(">");
            } else {
                display.setTextColor(WHITE);
            }

            const char* txt = items[li].GetTitle().c_str();
            int avail = (show_scroll ? 116 : 122);

            if (is_sel) {
                int tw = strlen(txt) * 6;
                if (tw > avail) {
                    // Cuon chu
                    if (now - last_txt > 80) {
                        txt_off++;
                        last_txt = now;
                    }
                    if (txt_off * 6 > tw) txt_off = -avail / 6;
                    display.setCursor(12 - txt_off * 6 > 12 ? 12 : 12 - txt_off * 6, iy + 2);
                } else {
                    display.setCursor(12, iy + 2);
                }
                display.print(txt);
            } else {
                display.setCursor(12, iy + 2);
                // In toi da so ky tu vua man hinh
                int max_c = avail / 6;
                for (int c = 0; c < max_c && txt[c]; c++)
                    display.write(txt[c]);
            }
            display.setTextColor(WHITE);
        }

        // ── Scrollbar ────────────────────────────────────────
        if (show_scroll) {
            int tx = 125, ty = HDR_H + 1, th = VISIBLE * ITEM_H;
            int bh = (VISIBLE * th) / items.size();
            int max_s = items.size() - VISIBLE;
            int by_ = (max_s > 0) ? (off * (th - bh)) / max_s : 0;
            display.drawFastVLine(tx, ty, th, WHITE);
            display.fillRect(tx, ty + by_, 2, bh > 2 ? bh : 2, WHITE);
        }

        // ── Footer ───────────────────────────────────────────
        display.drawFastHLine(0, FTR_Y - 1, 128, WHITE);
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.setCursor(2,         FTR_Y + 1); display.print("Len");
        display.setCursor(55,        FTR_Y + 1); display.print("OK");
        display.setCursor(128 - 38,  FTR_Y + 1); display.print("Xuong");

        display.setTextWrap(true);
        if (!skip_display) display.display();
    }

    const String& GetSelectedTitle() const {
        static String empty;
        if (items.empty()) return empty;
        return items[sel].GetTitle();
    }
};

