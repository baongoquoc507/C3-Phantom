#pragma once
/*
 * ============================================================
 *  BRICKS.HPP  —  Bat Bong Pha Tuong (Breakout)
 *  Dieu khien 3 nut:
 *    LEFT   = Di chuyen thanh trai (giu de lap)
 *    CENTER = Phong bong / Pause / Resume
 *    RIGHT  = Di chuyen thanh phai (giu de lap)
 * ============================================================
 */

#include "../global.hpp"

// ── Man hinh ────────────────────────────────────────────────
#define BK_W          128
#define BK_H          64

// ── Thanh (paddle) ──────────────────────────────────────────
#define PAD_W         20      // chieu rong thanh
#define PAD_H         3       // chieu cao thanh
#define PAD_Y         (BK_H - 6)  // y thanh
#define PAD_SPEED     3       // pixel moi nhan
#define PAD_REPEAT    70      // ms delay repeat

// ── Bong ────────────────────────────────────────────────────
#define BALL_R        2       // ban kinh bong
#define BALL_SPEED    2.2f    // toc do ban dau
#define BALL_MAX      4.0f    // toc do toi da
#define BALL_ACCEL    0.04f   // tang toc sau moi lan do

// ── Gach (brick) ────────────────────────────────────────────
#define BK_COLS       11      // so cot gach
#define BK_ROWS       5       // so hang gach
#define BK_BW         10      // rong moi gach (pixel)
#define BK_BH         4       // cao moi gach (pixel)
#define BK_GAP        1       // khoang giua gach
#define BK_TOP        8       // y bat dau khu gach
#define BK_LEFT       3       // x bat dau khu gach

// Tong gach: BK_COLS * BK_ROWS = 55

// ── Do ben gach theo hang (1=1hit, 2=2hit) ──────────────────
static const uint8_t BRICK_HP[BK_ROWS] = { 2, 2, 1, 1, 1 };

// ── Trang thai ──────────────────────────────────────────────
struct BKBall {
    float x, y;
    float vx, vy;
    bool  active;   // false = nam tren thanh chua phong
};

struct BKState {
    // Thanh
    float pad_x;    // x tam thanh
    // Bong
    BKBall ball;
    // Gach: hp[row][col], 0=vo
    uint8_t bricks[BK_ROWS][BK_COLS];
    int remaining;  // so gach con lai
    // Game
    int score;
    int lives;
    int level;
    bool paused;
    bool game_over;
    bool win;
    // Repeat nut
    unsigned long pad_l_t, pad_r_t;
};

static BKState bk;

// ── Tinh vi tri gach ─────────────────────────────────────────
static void BK_BrickRect(int r, int c, int* bx, int* by, int* bw, int* bh)
{
    *bx = BK_LEFT + c * (BK_BW + BK_GAP);
    *by = BK_TOP  + r * (BK_BH + BK_GAP);
    *bw = BK_BW;
    *bh = BK_BH;
}

// ── Khoi tao level ──────────────────────────────────────────
static void BK_InitLevel()
{
    bk.remaining = 0;
    for (int r = 0; r < BK_ROWS; r++)
        for (int c = 0; c < BK_COLS; c++) {
            bk.bricks[r][c] = BRICK_HP[r];
            bk.remaining++;
        }

    // Bong ngoi tren thanh
    bk.ball.x      = bk.pad_x;
    bk.ball.y      = PAD_Y - BALL_R - 1;
    bk.ball.vx     = 0;
    bk.ball.vy     = 0;
    bk.ball.active = false;
}

static void BK_Init()
{
    bk.pad_x    = BK_W / 2.0f;
    bk.score    = 0;
    bk.lives    = 3;
    bk.level    = 1;
    bk.paused   = false;
    bk.game_over = false;
    bk.win      = false;
    bk.pad_l_t  = bk.pad_r_t = 0;
    BK_InitLevel();
}

// ── Phong bong ───────────────────────────────────────────────
static void BK_Launch()
{
    float spd = BALL_SPEED + (bk.level - 1) * 0.2f;
    // Goc ngau nhien -45..+45 so voi thang dung
    float angle = ((random(0, 90) - 45) * 3.14159f / 180.0f);
    bk.ball.vx     = spd * sinf(angle);
    bk.ball.vy     = -spd;
    bk.ball.active = true;
}

// ── Va cham bong - gach ──────────────────────────────────────
static void BK_CheckBricks()
{
    float bx = bk.ball.x, by = bk.ball.y;
    for (int r = 0; r < BK_ROWS; r++) {
        for (int c = 0; c < BK_COLS; c++) {
            if (!bk.bricks[r][c]) continue;
            int gx, gy, gw, gh;
            BK_BrickRect(r, c, &gx, &gy, &gw, &gh);

            // AABB (mo rong them 1px vay bong)
            if (bx + BALL_R < gx || bx - BALL_R > gx + gw ||
                by + BALL_R < gy || by - BALL_R > gy + gh)
                continue;

            // Xac dinh mat va cham
            float overL = (bx + BALL_R) - gx;
            float overR = (gx + gw) - (bx - BALL_R);
            float overT = (by + BALL_R) - gy;
            float overB = (gy + gh) - (by - BALL_R);

            float minOv = min(min(overL, overR), min(overT, overB));

            if (minOv == overL || minOv == overR) bk.ball.vx = -bk.ball.vx;
            else                                   bk.ball.vy = -bk.ball.vy;

            // Giam HP gach
            bk.bricks[r][c]--;
            if (!bk.bricks[r][c]) {
                bk.remaining--;
                bk.score += (BRICK_HP[r] == 2) ? 20 : 10;
            } else {
                bk.score += 5;  // hit gach 2-HP lan 1
            }
            // Tang toc nhe
            float spd = sqrtf(bk.ball.vx*bk.ball.vx + bk.ball.vy*bk.ball.vy);
            if (spd < BALL_MAX) {
                float ns = spd + BALL_ACCEL;
                bk.ball.vx = bk.ball.vx / spd * ns;
                bk.ball.vy = bk.ball.vy / spd * ns;
            }
            return; // 1 va cham moi frame
        }
    }
}

// ── Ve ──────────────────────────────────────────────────────
static void BK_Draw()
{
    display.clearDisplay();

    // ── Gach ───────────────────────────────────────────────
    for (int r = 0; r < BK_ROWS; r++) {
        for (int c = 0; c < BK_COLS; c++) {
            if (!bk.bricks[r][c]) continue;
            int gx, gy, gw, gh;
            BK_BrickRect(r, c, &gx, &gy, &gw, &gh);
            if (bk.bricks[r][c] == 2) {
                // Gach ben: fill + outline kep
                display.fillRect(gx, gy, gw, gh, WHITE);
                display.drawRect(gx+1, gy+1, gw-2, gh-2, BLACK);
            } else {
                // Gach thuong: chi outline
                display.drawRect(gx, gy, gw, gh, WHITE);
            }
        }
    }

    // ── Thanh ──────────────────────────────────────────────
    int px = (int)(bk.pad_x - PAD_W / 2.0f);
    // Clamp
    if (px < 0) px = 0;
    if (px + PAD_W > BK_W) px = BK_W - PAD_W;
    display.fillRoundRect(px, PAD_Y, PAD_W, PAD_H, 1, WHITE);

    // ── Bong ───────────────────────────────────────────────
    display.fillCircle((int)bk.ball.x, (int)bk.ball.y, BALL_R, WHITE);

    // ── HUD phai ───────────────────────────────────────────
    display.setTextColor(WHITE);
    display.setTextSize(1);
    // Score
    display.setCursor(0, 0);
    display.print(bk.score);
    // Mang song
    display.setCursor(80, 0);
    for (int i = 0; i < bk.lives; i++) display.print("*");
    // Level
    display.setCursor(108, 0);
    display.print("L"); display.print(bk.level);

    // Pause overlay
    if (bk.paused) {
        display.fillRect(44, 24, 40, 16, BLACK);
        display.drawRect(44, 24, 40, 16, WHITE);
        display.setCursor(50, 28);
        display.print("PAUSE");
    }

    display.display();
}

// ── Ham chinh ────────────────────────────────────────────────
void Game_Bricks()
{
    BK_Init();

    // Man hinh san sang
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(30, 5);  display.print("BRICKS!");
    display.setCursor(5,  18); display.print("L/R = Di chuyen");
    display.setCursor(5,  28); display.print("CTR = Phong bong");
    display.setCursor(5,  38); display.print("CTR giu = Pause");
    display.setCursor(5,  50); display.print("Bam bat ky de bat dau");
    display.display();

    // Cho bam
    while (!(ReadButton(BUTTON_LEFT)||ReadButton(BUTTON_CENTER)||ReadButton(BUTTON_RIGHT))) delay(10);
    if (ReadButton(BUTTON_LEFT) && !ReadButton(BUTTON_CENTER) && !ReadButton(BUTTON_RIGHT)) {
        while (ReadButton(BUTTON_LEFT)) delay(5);
        return;  // LEFT don = thoat
    }
    while (ReadButton(BUTTON_LEFT)||ReadButton(BUTTON_CENTER)||ReadButton(BUTTON_RIGHT)) delay(5);

    const unsigned long TICK = 16;  // ~60fps
    unsigned long last = millis();
    static unsigned long center_down = 0;
    static bool center_held = false;

    while (true) {

        if (bk.game_over || bk.win) {
            display.clearDisplay();
            display.setTextColor(WHITE);
            display.setTextSize(1);
            if (bk.win) {
                display.setCursor(30, 8); display.print("CLEARED!");
            } else {
                display.setCursor(25, 8); display.print("GAME OVER");
            }
            display.drawRect(18, 22, 92, 24, WHITE);
            display.setCursor(24, 26); display.print("Score: "); display.print(bk.score);
            display.setCursor(24, 36); display.print("Level: "); display.print(bk.level);
            display.setCursor(10, 52); display.print("CTR=Lai   L=Menu");
            display.display();

            delay(300);
            while (ReadButton(BUTTON_LEFT)||ReadButton(BUTTON_CENTER)||ReadButton(BUTTON_RIGHT)) delay(5);
            while (true) {
                if (ReadButton(BUTTON_CENTER)||ReadButton(BUTTON_RIGHT)) {
                    while (ReadButton(BUTTON_CENTER)||ReadButton(BUTTON_RIGHT)) delay(5);
                    BK_Init();
                    last = millis();
                    break;
                }
                if (ReadButton(BUTTON_LEFT)) {
                    while (ReadButton(BUTTON_LEFT)) delay(5);
                    return;
                }
                delay(10);
            }
            continue;
        }

        unsigned long now = millis();

        // ── Xu ly nut ─────────────────────────────────────
        bool btnL = ReadButton(BUTTON_LEFT);
        bool btnC = ReadButton(BUTTON_CENTER);
        bool btnR = ReadButton(BUTTON_RIGHT);

        // CENTER: tap = phong / giu 400ms = pause
        if (btnC) {
            if (!center_held) { center_held = true; center_down = now; }
            if (now - center_down > 400 && !bk.paused) {
                bk.paused = !bk.paused;
                while (ReadButton(BUTTON_CENTER)) delay(5);
                center_held = false;
            }
        } else {
            if (center_held) {
                if (now - center_down < 400) {
                    if (bk.paused) {
                        bk.paused = false;   // Resume
                    } else if (!bk.ball.active) {
                        BK_Launch();          // Phong bong
                    }
                }
                center_held = false;
            }
        }

        if (bk.paused) { BK_Draw(); delay(10); continue; }

        // Di chuyen thanh (giu de lap nhanh)
        if (btnL) {
            if (now - bk.pad_l_t > (bk.pad_l_t == 0 ? 0 : PAD_REPEAT)) {
                bk.pad_l_t = now;
                bk.pad_x -= PAD_SPEED;
                if (bk.pad_x - PAD_W/2 < 0) bk.pad_x = PAD_W/2.0f;
            }
        } else { bk.pad_l_t = 0; }

        if (btnR) {
            if (now - bk.pad_r_t > (bk.pad_r_t == 0 ? 0 : PAD_REPEAT)) {
                bk.pad_r_t = now;
                bk.pad_x += PAD_SPEED;
                if (bk.pad_x + PAD_W/2 > BK_W) bk.pad_x = BK_W - PAD_W/2.0f;
            }
        } else { bk.pad_r_t = 0; }

        // Kiem tra tick
        if (now - last < TICK) { delay(1); continue; }
        last = now;

        // ── Bong ngoi tren thanh ──────────────────────────
        if (!bk.ball.active) {
            bk.ball.x = bk.pad_x;
            bk.ball.y = PAD_Y - BALL_R - 1;
            BK_Draw();
            continue;
        }

        // ── Di chuyen bong ────────────────────────────────
        bk.ball.x += bk.ball.vx;
        bk.ball.y += bk.ball.vy;

        // ── Bien ngang ────────────────────────────────────
        if (bk.ball.x - BALL_R < 0) {
            bk.ball.x  = BALL_R;
            bk.ball.vx = fabsf(bk.ball.vx);
        }
        if (bk.ball.x + BALL_R > BK_W) {
            bk.ball.x  = BK_W - BALL_R;
            bk.ball.vx = -fabsf(bk.ball.vx);
        }

        // ── Tran ─────────────────────────────────────────
        if (bk.ball.y - BALL_R < 0) {
            bk.ball.y  = BALL_R;
            bk.ball.vy = fabsf(bk.ball.vy);
        }

        // ── Va cham thanh ─────────────────────────────────
        float px = bk.pad_x;
        if (bk.ball.y + BALL_R >= PAD_Y &&
            bk.ball.y - BALL_R <= PAD_Y + PAD_H &&
            bk.ball.x >= px - PAD_W/2 - BALL_R &&
            bk.ball.x <= px + PAD_W/2 + BALL_R)
        {
            // Dieu huong theo vi tri cham tren thanh
            float offset = (bk.ball.x - px) / (PAD_W / 2.0f);
            float spd = sqrtf(bk.ball.vx*bk.ball.vx + bk.ball.vy*bk.ball.vy);
            float maxAngle = 65.0f * 3.14159f / 180.0f;
            float angle    = offset * maxAngle;
            bk.ball.vx = spd * sinf(angle);
            bk.ball.vy = -fabsf(spd * cosf(angle));
            bk.ball.y  = PAD_Y - BALL_R - 1;
        }

        // ── Roi xuong ─────────────────────────────────────
        if (bk.ball.y - BALL_R > BK_H) {
            bk.lives--;
            if (bk.lives <= 0) {
                bk.game_over = true;
            } else {
                bk.ball.active = false;
                bk.ball.x = bk.pad_x;
                bk.ball.y = PAD_Y - BALL_R - 1;
            }
            BK_Draw();
            if (!bk.game_over) {
                // Nha nut roi tiep tuc
                delay(400);
            }
            continue;
        }

        // ── Va cham gach ─────────────────────────────────
        BK_CheckBricks();

        // ── Win? ─────────────────────────────────────────
        if (bk.remaining <= 0) {
            bk.level++;
            if (bk.level > 5) {
                bk.win = true;
            } else {
                // Next level: gach moi, giu thanh & bong & score
                BK_InitLevel();
                bk.ball.active = false;
                display.clearDisplay();
                display.setTextColor(WHITE);
                display.setCursor(25, 20); display.print("LEVEL ");
                display.print(bk.level);
                display.setCursor(20, 35); display.print("CTR = Tiep tuc");
                display.display();
                while (!(ReadButton(BUTTON_CENTER)||ReadButton(BUTTON_RIGHT))) delay(10);
                while (ReadButton(BUTTON_CENTER)||ReadButton(BUTTON_RIGHT)) delay(5);
            }
            continue;
        }

        BK_Draw();
    }
}
