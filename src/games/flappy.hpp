#pragma once
/*
 * ============================================================
 *  FLAPPY.HPP  —  Flappy Bird / Cham De Bay
 *  Dieu khien 3 nut:
 *    LEFT   = Tap de bay / thoat khi game over
 *    CENTER = Tap de bay (nut chinh)
 *    RIGHT  = Tap de bay
 *  3 nut deu lam chim vot len — bo tap toan man hinh
 * ============================================================
 */

#include "../global.hpp"

// ── Hang so man hinh ────────────────────────────────────────
#define FL_W         128
#define FL_H         64

// ── Chim ────────────────────────────────────────────────────
#define BIRD_X       22       // x co dinh cua chim
#define BIRD_R       4        // ban kinh hit-box chim (pixel)
#define GRAVITY_F    0.26f    // gia toc roi (pixel/frame^2)
#define FLAP_VEL    -2.8f     // van toc vot len khi tap
#define BIRD_MAX_VEL 3.5f     // gioi han toc do roi

// ── Ong ─────────────────────────────────────────────────────
#define PIPE_W       10       // chieu rong ong (pixel)
#define PIPE_GAP     23       // khoang trong giua 2 ong (pixel)
#define PIPE_SPEED   1.0f     // toc do cuon ngang (pixel/frame)
#define PIPE_SPACE   65       // khoang cach giua 2 cap ong
#define NUM_PIPES    3        // so cap ong tren man hinh

// ── Mat dat / tran ──────────────────────────────────────────
#define GROUND_FL    (FL_H - 5)   // y mat dat
#define CEIL_FL      4             // y tran

// ── Trang thai ──────────────────────────────────────────────
struct FlappyPipe {
    float x;
    int   gap_y;    // y trung tam khe ho
    bool  scored;   // da tinh diem chua
};

struct FlappyGame {
    float  bird_y;
    float  bird_vy;
    int    score;
    int    hi_score;
    bool   game_over;
    bool   started;
    FlappyPipe pipes[NUM_PIPES];
    int    anim;          // frame canh 0/1
    unsigned long anim_t;
    // Ground scroll
    float  gnd_off;
};

static FlappyGame fl;

// ── Sprite chim (7x5) ───────────────────────────────────────
// Frame 0: canh duoi  Frame 1: canh tren
static const uint8_t BIRD_F0[] PROGMEM = {
    0b00111000,
    0b01111100,
    0b11111010,
    0b01111100,
    0b00110000,
};
static const uint8_t BIRD_F1[] PROGMEM = {
    0b00110000,
    0b01111100,
    0b11111010,
    0b01111100,
    0b00111000,
};

// ── Khoi tao ─────────────────────────────────────────────────
static void FL_Init()
{
    fl.bird_y   = FL_H / 2.0f;
    fl.bird_vy  = 0;
    fl.score    = 0;
    fl.game_over = false;
    fl.started  = false;
    fl.anim     = 0;
    fl.anim_t   = millis();
    fl.gnd_off  = 0;

    for (int i = 0; i < NUM_PIPES; i++) {
        fl.pipes[i].x      = FL_W + 20 + i * PIPE_SPACE;
        fl.pipes[i].gap_y  = 18 + random(28);   // 18..45
        fl.pipes[i].scored = false;
    }
}

// ── Va cham ─────────────────────────────────────────────────
static bool FL_Hit()
{
    // Chi chet khi roi xuong dat (khong chet vi tuong/tran)
    if (fl.bird_y + BIRD_R >= GROUND_FL) return true;

    // Chet khi cham ong
    for (int i = 0; i < NUM_PIPES; i++) {
        float px = fl.pipes[i].x;
        if (BIRD_X + BIRD_R > px && BIRD_X - BIRD_R < px + PIPE_W) {
            int gy = fl.pipes[i].gap_y;
            if (fl.bird_y - BIRD_R < gy - PIPE_GAP / 2) return true;
            if (fl.bird_y + BIRD_R > gy + PIPE_GAP / 2) return true;
        }
    }
    return false;
}

// ── Ve ──────────────────────────────────────────────────────
static void FL_Draw()
{
    display.clearDisplay();

    // ── Ong ────────────────────────────────────────────────
    for (int i = 0; i < NUM_PIPES; i++) {
        int px  = (int)fl.pipes[i].x;
        int gy  = fl.pipes[i].gap_y;
        int top = gy - PIPE_GAP / 2;    // day ong tren
        int bot = gy + PIPE_GAP / 2;    // dinh ong duoi

        // Ong tren (CEIL_FL → top)
        display.fillRect(px, CEIL_FL, PIPE_W, top - CEIL_FL, WHITE);
        // Mui ong tren (2px rong hon 2 ben)
        display.fillRect(px - 2, top - 3, PIPE_W + 4, 3, WHITE);

        // Ong duoi (bot → GROUND_FL)
        // Mui ong duoi
        display.fillRect(px - 2, bot, PIPE_W + 4, 3, WHITE);
        display.fillRect(px, bot + 3, PIPE_W, GROUND_FL - bot - 3, WHITE);
    }

    // ── Mat dat (cuon) ─────────────────────────────────────
    display.drawFastHLine(0, GROUND_FL, FL_W, WHITE);
    int goff = (int)fl.gnd_off % 8;
    for (int x = -goff; x < FL_W; x += 8)
        display.drawPixel(x + 4, GROUND_FL + 2, WHITE);

    // ── Tran ───────────────────────────────────────────────
    display.drawFastHLine(0, CEIL_FL, FL_W, WHITE);

    // ── Chim ───────────────────────────────────────────────
    const uint8_t* sp = (fl.anim == 0) ? BIRD_F0 : BIRD_F1;
    int bx = BIRD_X - 3;
    int by = (int)fl.bird_y - 2;
    // Clip ve man hinh
    if (by < 0) by = 0;
    if (by > FL_H - 5) by = FL_H - 5;
    display.drawBitmap(bx, by, sp, 7, 5, WHITE);

    // ── HUD ────────────────────────────────────────────────
    display.setTextColor(WHITE);
    display.setTextSize(1);
    // Score chinh giua phia tren
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", fl.score);
    int sw = strlen(buf) * 6;
    display.setCursor((FL_W - sw) / 2, 7);
    display.print(buf);

    // Hi score goc phai
    display.setCursor(90, 0);
    display.print("HI:");
    display.print(fl.hi_score);

    display.display();
}

static void FL_DrawReady()
{
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(1);

    // Ve ong mau
    display.fillRect(80, CEIL_FL, PIPE_W, 18, WHITE);
    display.fillRect(78, 15, PIPE_W+4, 3, WHITE);
    display.fillRect(78, 36, PIPE_W+4, 3, WHITE);
    display.fillRect(80, 39, PIPE_W, GROUND_FL-39, WHITE);
    display.drawFastHLine(0, GROUND_FL, FL_W, WHITE);
    display.drawFastHLine(0, CEIL_FL,   FL_W, WHITE);

    // Chim
    display.drawBitmap(BIRD_X-3, FL_H/2-2, BIRD_F0, 7, 5, WHITE);

    // Text
    display.setCursor(35, 10);
    display.setTextSize(1);
    display.print("FLAPPY");
    display.setCursor(28, 23);
    display.print("Cham de bay!");
    display.setCursor(20, 36);
    display.print("L/C/R = Vot len");
    display.setCursor(20, 47);
    display.print("Bam de bat dau");

    display.display();
}

static void FL_DrawGameOver()
{
    // Luu hi score
    if (fl.score > fl.hi_score) fl.hi_score = fl.score;

    // Blink 3 lan
    for (int b = 0; b < 3; b++) {
        display.invertDisplay(true);  delay(80);
        display.invertDisplay(false); delay(80);
    }

    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(28, 5);  display.print("GAME OVER");

    // Score box
    display.drawRect(24, 18, 80, 30, WHITE);
    display.setCursor(30, 23); display.print("Score: "); display.print(fl.score);
    display.setCursor(30, 33); display.print("Best:  "); display.print(fl.hi_score);

    display.setCursor(15, 54); display.print("Tap=Lai   L=Menu");
    display.display();
}

// ── Ham chinh ────────────────────────────────────────────────
void Game_Flappy()
{
    int saved_hi = fl.hi_score;   // giu hi score qua lan choi
    FL_Init();
    fl.hi_score = saved_hi;

    // Man hinh san sang
    FL_DrawReady();
    while (true) {
        if (ReadButton(BUTTON_LEFT) || ReadButton(BUTTON_CENTER) || ReadButton(BUTTON_RIGHT)) {
            // Nut Left -> thoat menu neu chua bat dau
            if (ReadButton(BUTTON_LEFT) && !fl.started) {
                // Cho nha nut roi thoat
                while (ReadButton(BUTTON_LEFT)) delay(5);
                return;
            }
            break;
        }
        delay(10);
    }
    while (ReadButton(BUTTON_LEFT) || ReadButton(BUTTON_CENTER) || ReadButton(BUTTON_RIGHT)) delay(5);

    FL_Init();
    fl.hi_score  = saved_hi;
    fl.started   = true;
    fl.bird_vy   = FLAP_VEL;  // vot len ngay khi bat dau

    const unsigned long TICK = 20;  // 50fps
    unsigned long last = millis();

    while (true) {
        // ── Ghi nhan nut ─────────────────────────────────
        bool tapped = false;
        if (ReadButton(BUTTON_LEFT) || ReadButton(BUTTON_CENTER) || ReadButton(BUTTON_RIGHT))
            tapped = true;

        if (fl.game_over) {
            FL_DrawGameOver();
            delay(300);
            // Cho nha het nut
            while (ReadButton(BUTTON_LEFT) || ReadButton(BUTTON_CENTER) || ReadButton(BUTTON_RIGHT)) delay(5);
            // Doi bam
            while (true) {
                if (ReadButton(BUTTON_CENTER) || ReadButton(BUTTON_RIGHT)) {
                    // Choi lai
                    while (ReadButton(BUTTON_CENTER) || ReadButton(BUTTON_RIGHT)) delay(5);
                    int hi = fl.hi_score;
                    FL_Init();
                    fl.hi_score = hi;
                    fl.started  = true;
                    fl.bird_vy  = FLAP_VEL;
                    last = millis();
                    break;
                }
                if (ReadButton(BUTTON_LEFT)) {
                    while (ReadButton(BUTTON_LEFT)) delay(5);
                    return;  // Thoat ve menu
                }
                delay(10);
            }
            continue;
        }

        // ── Vot len ──────────────────────────────────────
        if (tapped) {
            fl.bird_vy = FLAP_VEL;
            while (ReadButton(BUTTON_LEFT) || ReadButton(BUTTON_CENTER) || ReadButton(BUTTON_RIGHT)) delay(3);
        }

        // ── Kiem tra tick ────────────────────────────────
        unsigned long now = millis();
        if (now - last < TICK) { delay(2); continue; }
        last = now;

        // ── Vat ly chim ──────────────────────────────────
        fl.bird_vy += GRAVITY_F;
        if (fl.bird_vy > BIRD_MAX_VEL) fl.bird_vy = BIRD_MAX_VEL;
        fl.bird_y  += fl.bird_vy;
        // Cham tran: nay xuong (khong chet)
        if (fl.bird_y - BIRD_R < CEIL_FL) {
            fl.bird_y  = CEIL_FL + BIRD_R;
            fl.bird_vy = fabsf(fl.bird_vy) * 0.4f;
        }

        // ── Cuon ong ─────────────────────────────────────
        for (int i = 0; i < NUM_PIPES; i++) {
            fl.pipes[i].x -= PIPE_SPEED;

            // Tinh diem
            if (!fl.pipes[i].scored && fl.pipes[i].x + PIPE_W < BIRD_X) {
                fl.pipes[i].scored = true;
                fl.score++;
            }

            // Tai su dung ong
            if (fl.pipes[i].x + PIPE_W < 0) {
                // Dat lai sau ong cuoi cung
                float max_x = 0;
                for (int j = 0; j < NUM_PIPES; j++)
                    if (fl.pipes[j].x > max_x) max_x = fl.pipes[j].x;
                fl.pipes[i].x      = max_x + PIPE_SPACE;
                // Tang thu thach theo diem: khe hep dan tu 20→14
                int gap_half = max(7, 10 - fl.score / 8);
                fl.pipes[i].gap_y  = (CEIL_FL + gap_half + 5) +
                                     random((GROUND_FL - CEIL_FL) - 2*(gap_half+5));
                fl.pipes[i].scored = false;
            }
        }

        // ── Mat dat cuon ─────────────────────────────────
        fl.gnd_off += PIPE_SPEED;

        // ── Animation canh ───────────────────────────────
        unsigned long nt = millis();
        if (nt - fl.anim_t > 100) {
            fl.anim   ^= 1;
            fl.anim_t  = nt;
        }

        // ── Va cham ──────────────────────────────────────
        if (FL_Hit()) fl.game_over = true;

        // ── Ve ───────────────────────────────────────────
        FL_Draw();
    }
}
