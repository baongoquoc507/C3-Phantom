#pragma once
/*
 * ============================================================
 *  TETRIS.HPP  —  Game Xep Gach (Tetris)
 *  Dieu khien 3 nut:
 *    LEFT   = Xoay khoi
 *    CENTER = Roi nhanh (Hard Drop)
 *    RIGHT  = Di chuyen phai
 *  Logic: khoi tu dong di trai khi LEFT khong bam
 *         → tu di trai / RIGHT di phai / LEFT xoay
 * ============================================================
 *  Dieu khien thay the (de choi thuan tien hon):
 *    LEFT   held = di trai
 *    RIGHT  held = di phai
 *    CENTER tap  = xoay
 *    CENTER hold = roi nhanh
 * ============================================================
 */

#include "../global.hpp"

// ── Kich thuoc ban choi ─────────────────────────────────────
#define TET_W       10      // so cot
#define TET_H       18      // so hang
#define TET_CELL    3       // pixel/o (3px → 30x54 cho ban choi)
#define TET_BOARD_X 48      // vi tri x cua ban choi tren OLED (128-30 = 98? => dat trai)
#define TET_BOARD_Y 0       // vi tri y bat dau ban choi
// Ban choi: x = 0, rong = TET_W*TET_CELL = 30, cao = TET_H*TET_CELL = 54
// Khu hien thi diem: x = 32..127

// ── 7 loai khoi Tetromino ───────────────────────────────────
// Moi khoi: 4 trang thai xoay, moi trang thai 4 o (dx,dy)
static const int8_t TETROMINOS[7][4][4][2] = {
    // I - ngang
    { {{0,1},{1,1},{2,1},{3,1}}, {{2,0},{2,1},{2,2},{2,3}},
      {{0,2},{1,2},{2,2},{3,2}}, {{1,0},{1,1},{1,2},{1,3}} },
    // O - vuong
    { {{1,0},{2,0},{1,1},{2,1}}, {{1,0},{2,0},{1,1},{2,1}},
      {{1,0},{2,0},{1,1},{2,1}}, {{1,0},{2,0},{1,1},{2,1}} },
    // T
    { {{1,0},{0,1},{1,1},{2,1}}, {{1,0},{1,1},{2,1},{1,2}},
      {{0,1},{1,1},{2,1},{1,2}}, {{1,0},{0,1},{1,1},{1,2}} },
    // S
    { {{1,0},{2,0},{0,1},{1,1}}, {{1,0},{1,1},{2,1},{2,2}},
      {{1,1},{2,1},{0,2},{1,2}}, {{0,0},{0,1},{1,1},{1,2}} },
    // Z
    { {{0,0},{1,0},{1,1},{2,1}}, {{2,0},{1,1},{2,1},{1,2}},
      {{0,1},{1,1},{1,2},{2,2}}, {{1,0},{0,1},{1,1},{0,2}} },
    // J
    { {{0,0},{0,1},{1,1},{2,1}}, {{1,0},{2,0},{1,1},{1,2}},
      {{0,1},{1,1},{2,1},{2,2}}, {{1,0},{1,1},{0,2},{1,2}} },
    // L
    { {{2,0},{0,1},{1,1},{2,1}}, {{1,0},{1,1},{1,2},{2,2}},
      {{0,1},{1,1},{2,1},{0,2}}, {{0,0},{1,0},{1,1},{1,2}} }
};

// ── Trang thai game ─────────────────────────────────────────
struct TetrisGame {
    uint8_t board[TET_H][TET_W];   // 0 = trong, 1-7 = loai khoi
    int     px, py;                 // vi tri khoi hien tai
    int     ptype, prot;            // loai va goc xoay
    int     score;
    int     lines;
    int     level;
    bool    game_over;
    unsigned long last_fall;        // lan roi cuoi
    unsigned long fall_interval;    // ms moi buoc roi

    // Preview khoi tiep theo
    int next_type;
    // Repeat delay cho nut giu
    unsigned long btn_last_l, btn_last_r;
};

static TetrisGame tet;

// ── Ham tro giup ────────────────────────────────────────────

static bool Tet_Valid(int px, int py, int type, int rot)
{
    for (int i = 0; i < 4; i++) {
        int nx = px + TETROMINOS[type][rot][i][0];
        int ny = py + TETROMINOS[type][rot][i][1];
        if (nx < 0 || nx >= TET_W || ny >= TET_H) return false;
        if (ny >= 0 && tet.board[ny][nx]) return false;
    }
    return true;
}

static void Tet_Place()
{
    for (int i = 0; i < 4; i++) {
        int nx = tet.px + TETROMINOS[tet.ptype][tet.prot][i][0];
        int ny = tet.py + TETROMINOS[tet.ptype][tet.prot][i][1];
        if (ny >= 0) tet.board[ny][nx] = (uint8_t)(tet.ptype + 1);
    }
}

static int Tet_ClearLines()
{
    int cleared = 0;
    for (int y = TET_H - 1; y >= 0; y--) {
        bool full = true;
        for (int x = 0; x < TET_W; x++)
            if (!tet.board[y][x]) { full = false; break; }
        if (full) {
            cleared++;
            // Dich xuong
            for (int yy = y; yy > 0; yy--)
                for (int x = 0; x < TET_W; x++)
                    tet.board[yy][x] = tet.board[yy-1][x];
            for (int x = 0; x < TET_W; x++) tet.board[0][x] = 0;
            y++; // kiem tra lai dong nay
        }
    }
    return cleared;
}

static void Tet_Spawn()
{
    tet.ptype = tet.next_type;
    tet.prot  = 0;
    tet.px    = 3;
    tet.py    = -1;
    tet.next_type = random(7);

    if (!Tet_Valid(tet.px, tet.py, tet.ptype, tet.prot)) {
        tet.game_over = true;
    }
}

static void Tet_Init()
{
    memset(tet.board, 0, sizeof(tet.board));
    tet.score    = 0;
    tet.lines    = 0;
    tet.level    = 1;
    tet.game_over = false;
    tet.last_fall = millis();
    tet.fall_interval = 600;
    tet.next_type = random(7);
    tet.btn_last_l = tet.btn_last_r = 0;
    Tet_Spawn();
}

// Hard drop
static void Tet_HardDrop()
{
    while (Tet_Valid(tet.px, tet.py + 1, tet.ptype, tet.prot))
        tet.py++;
    Tet_Place();
    int c = Tet_ClearLines();
    tet.lines += c;
    int pts[] = {0, 40, 100, 300, 1200};
    tet.score += pts[c] * tet.level;
    tet.level = tet.lines / 10 + 1;
    tet.fall_interval = max(60, 600 - (tet.level - 1) * 50);
    Tet_Spawn();
}

// ── Ve man hinh ─────────────────────────────────────────────
static void Tet_Draw()
{
    display.clearDisplay();

    // ── Khung ban choi ────────────────────────────────────
    int bx = 0, by = 0;
    int bw = TET_W * TET_CELL;   // 30
    int bh = TET_H * TET_CELL;   // 54
    display.drawRect(bx - 1, by - 1, bw + 2, bh + 2, WHITE);

    // ── Ve bang ───────────────────────────────────────────
    for (int y = 0; y < TET_H; y++) {
        for (int x = 0; x < TET_W; x++) {
            if (tet.board[y][x]) {
                display.fillRect(bx + x * TET_CELL, by + y * TET_CELL,
                                 TET_CELL - 1, TET_CELL - 1, WHITE);
            }
        }
    }

    // ── Ve Ghost piece (bong mo) ───────────────────────────
    int ghost_y = tet.py;
    while (Tet_Valid(tet.px, ghost_y + 1, tet.ptype, tet.prot)) ghost_y++;
    if (ghost_y != tet.py) {
        for (int i = 0; i < 4; i++) {
            int gx = tet.px + TETROMINOS[tet.ptype][tet.prot][i][0];
            int gy = ghost_y + TETROMINOS[tet.ptype][tet.prot][i][1];
            if (gy >= 0)
                display.drawRect(bx + gx * TET_CELL, by + gy * TET_CELL,
                                 TET_CELL - 1, TET_CELL - 1, WHITE);
        }
    }

    // ── Ve khoi hien tai ──────────────────────────────────
    for (int i = 0; i < 4; i++) {
        int cx = tet.px + TETROMINOS[tet.ptype][tet.prot][i][0];
        int cy = tet.py + TETROMINOS[tet.ptype][tet.prot][i][1];
        if (cy >= 0)
            display.fillRect(bx + cx * TET_CELL, by + cy * TET_CELL,
                             TET_CELL - 1, TET_CELL - 1, WHITE);
    }

    // ── Panel phai ────────────────────────────────────────
    int px2 = 34; // x bat dau panel phai
    display.setTextSize(1);
    display.setTextColor(WHITE);

    // Header
    display.setCursor(px2, 0);
    display.print("TETRIS");

    // Score
    display.setCursor(px2, 12);
    display.print("PTS");
    display.setCursor(px2, 21);
    display.print(tet.score);

    // Level
    display.setCursor(px2, 33);
    display.print("LVL");
    display.setCursor(px2, 42);
    display.print(tet.level);

    // Lines
    display.setCursor(px2, 54);
    display.print("L:");
    display.print(tet.lines);

    // Preview next piece (mini 3x3 area)
    // Ve khoi tiep theo o goc phai tren
    int nx_base_x = 94, nx_base_y = 2;
    display.drawRect(nx_base_x - 1, nx_base_y - 1, 18, 18, WHITE);
    for (int i = 0; i < 4; i++) {
        int nx = TETROMINOS[tet.next_type][0][i][0];
        int ny = TETROMINOS[tet.next_type][0][i][1];
        if (nx < 4 && ny < 4)
            display.fillRect(nx_base_x + nx * 4, nx_base_y + ny * 4, 3, 3, WHITE);
    }

    // Huong dan nut
    display.setCursor(33, 44);
    display.print("L=Xoay");
    display.setCursor(33, 54);
    display.print("C=Drop");

    display.display();
}

static void Tet_DrawGameOver()
{
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(22, 10);
    display.print("GAME OVER");
    display.setCursor(10, 25);
    display.print("Score: ");
    display.print(tet.score);
    display.setCursor(10, 37);
    display.print("Lines: ");
    display.print(tet.lines);
    display.setCursor(10, 49);
    display.print("Level: ");
    display.print(tet.level);
    display.setCursor(10, 58);
    display.print("CTR=Choi lai");
    display.display();
}

// ── Ham chinh game ───────────────────────────────────────────
void Game_Tetris()
{
    Tet_Init();

    unsigned long btn_repeat_l = 0, btn_repeat_r = 0;
    const unsigned long BTN_DELAY  = 180;
    const unsigned long BTN_REPEAT = 100;

    while (true) {
        if (tet.game_over) {
            Tet_DrawGameOver();
            delay(300);
            HaltTillRelease(BUTTON_CENTER);
            // Cho bam CENTER de choi lai hoac LEFT/RIGHT de thoat
            while (true) {
                if (ReadButtonWait(BUTTON_CENTER)) { Tet_Init(); break; }
                if (ReadButtonWait(BUTTON_LEFT))  return;
                if (ReadButtonWait(BUTTON_RIGHT)) return;
                delay(10);
            }
            continue;
        }

        unsigned long now = millis();

        // ── Xu ly nut bam ───────────────────────────────────
        // LEFT: Xoay
        if (ReadButton(BUTTON_LEFT)) {
            if (now - btn_repeat_l > BTN_DELAY) {
                btn_repeat_l = now;
                int nr = (tet.prot + 1) % 4;
                if (Tet_Valid(tet.px, tet.py, tet.ptype, nr))
                    tet.prot = nr;
                else if (Tet_Valid(tet.px - 1, tet.py, tet.ptype, nr))
                    { tet.prot = nr; tet.px--; }
                else if (Tet_Valid(tet.px + 1, tet.py, tet.ptype, nr))
                    { tet.prot = nr; tet.px++; }
            }
        } else { btn_repeat_l = 0; }

        // RIGHT: Di chuyen phai
        if (ReadButton(BUTTON_RIGHT)) {
            if (now - btn_repeat_r > BTN_DELAY) {
                btn_repeat_r = now;
                if (Tet_Valid(tet.px + 1, tet.py, tet.ptype, tet.prot))
                    tet.px++;
            }
        } else { btn_repeat_r = 0; }

        // CENTER: Hard drop (nhan tap) hoac di chuyen trai (giu)
        // → CENTER giu > 300ms = di trai
        static bool center_held = false;
        static unsigned long center_t = 0;
        if (ReadButton(BUTTON_CENTER)) {
            if (!center_held) { center_held = true; center_t = now; }
            if (now - center_t > 300) {
                // Giu = di trai
                static unsigned long cleft = 0;
                if (now - cleft > BTN_REPEAT) {
                    cleft = now;
                    if (Tet_Valid(tet.px - 1, tet.py, tet.ptype, tet.prot))
                        tet.px--;
                }
            }
        } else {
            if (center_held && (now - center_t < 300)) {
                // Tap = hard drop
                Tet_HardDrop();
            }
            center_held = false;
        }

        // ── Roi tu dong ─────────────────────────────────────
        if (now - tet.last_fall >= tet.fall_interval) {
            tet.last_fall = now;
            if (Tet_Valid(tet.px, tet.py + 1, tet.ptype, tet.prot)) {
                tet.py++;
            } else {
                Tet_Place();
                int c = Tet_ClearLines();
                tet.lines += c;
                int pts[] = {0, 40, 100, 300, 1200};
                tet.score += pts[c] * tet.level;
                tet.level = tet.lines / 10 + 1;
                tet.fall_interval = max(60, 600 - (tet.level - 1) * 50);
                Tet_Spawn();
            }
        }

        Tet_Draw();
        delay(16); // ~60fps
    }
}
