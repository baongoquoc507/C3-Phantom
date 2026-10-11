#pragma once
/*
 * ============================================================
 *  DINO.HPP  —  Khung Long Nhay / Dino Runner
 *  Dieu khien 3 nut:
 *    LEFT   = Cro xuong (duck) / nhanh hon
 *    CENTER = Nhay
 *    RIGHT  = Nhay doi (double jump)
 *
 *  Khu vuc choi: toan man hinh 128x64
 *  Khung long chay o x = 20, mat dat y = 52
 * ============================================================
 */

#include "../global.hpp"

// ── Hang so ─────────────────────────────────────────────────
#define DINO_X          14      // vi tri x khung long (co dinh)
#define GROUND_Y        52      // y mat dat (chan khung long)
#define DINO_W          12      // chieu rong sprite khung long
#define DINO_H          13      // chieu cao sprite khung long (dung)
#define DINO_DUCK_H     8       // chieu cao khi cro
#define GRAVITY         1.8f    // gia toc roi (pixel/tick^2)
#define JUMP_VEL       -7.5f    // van toc nhay
#define DOUBLE_JUMP_VEL -5.5f   // van toc nhay doi
#define GAME_TICK       20      // ms moi frame (50fps)
#define OBS_MIN_GAP     35      // khoang cach toi thieu giua vat can
#define OBS_MAX_GAP     80      // khoang cach toi da

// ── Sprite khung long (12x13) ─ bitmask ────────────────────
// Moi byte = 1 hang, bit MSB = cot trai
// Dung cho drawBitmap de hien thi khung lon
static const uint8_t DINO_SPRITE_RUN1[] PROGMEM = {
    0b00011111, 0b11000000,  // ..###########
    0b00011111, 0b11100000,
    0b00011111, 0b11110000,
    0b00011111, 0b11000000,
    0b01111111, 0b10000000,
    0b11111110, 0b00000000,
    0b11111111, 0b00000000,
    0b01111100, 0b00000000,
    0b00111100, 0b00000000,
    0b00110100, 0b00000000,
    0b00110000, 0b00000000,
    0b01110000, 0b00000000,
    0b01100000, 0b00000000,
};

static const uint8_t DINO_SPRITE_RUN2[] PROGMEM = {
    0b00011111, 0b11000000,
    0b00011111, 0b11100000,
    0b00011111, 0b11110000,
    0b00011111, 0b11000000,
    0b01111111, 0b10000000,
    0b11111110, 0b00000000,
    0b11111111, 0b00000000,
    0b01111100, 0b00000000,
    0b00111100, 0b00000000,
    0b00110100, 0b00000000,
    0b00100110, 0b00000000,
    0b00000110, 0b00000000,
    0b00000111, 0b00000000,
};

// Sprite cro (12x8)
static const uint8_t DINO_SPRITE_DUCK[] PROGMEM = {
    0b00000011, 0b11110000,
    0b00011111, 0b11110000,
    0b00011111, 0b11110000,
    0b11111110, 0b01000000,
    0b11111111, 0b00000000,
    0b01111100, 0b00000000,
    0b01010000, 0b00000000,
    0b01100000, 0b00000000,
};

// ── Sprite xuong rong (cactus) ──────────────────────────────
// Xuong rong nho (8x16)
static const uint8_t CACTUS_S[] PROGMEM = {
    0b00100000,
    0b00100000,
    0b10100000,
    0b11100000,
    0b10111000,
    0b00111000,
    0b00111000,
    0b00111000,
    0b00111000,
    0b00111000,
    0b00100000,
    0b00100000,
    0b00100000,
    0b00100000,
    0b00100000,
    0b00100000,
};

// Xuong rong lon (12x20)
static const uint8_t CACTUS_L[] PROGMEM = {
    0b00011000, 0b00000000,
    0b00011000, 0b00000000,
    0b10011000, 0b00000000,
    0b11011110, 0b00000000,
    0b11111110, 0b00000000,
    0b11111110, 0b00000000,
    0b01111110, 0b00000000,
    0b00011000, 0b00000000,
    0b00011000, 0b00000000,
    0b00011000, 0b00000000,
    0b00011001, 0b10000000,
    0b00011001, 0b10000000,
    0b00011111, 0b10000000,
    0b00011111, 0b10000000,
    0b00011111, 0b10000000,
    0b00011111, 0b10000000,
    0b00011000, 0b00000000,
    0b00011000, 0b00000000,
    0b00011000, 0b00000000,
    0b00011000, 0b00000000,
};

// Chim (8x5) bay tren troi
static const uint8_t BIRD_SPRITE1[] PROGMEM = {
    0b00001100,
    0b11111110,
    0b11110000,
    0b01110000,
    0b00110000,
};
static const uint8_t BIRD_SPRITE2[] PROGMEM = {
    0b11000000,
    0b11111110,
    0b00111111,
    0b00011110,
    0b00001100,
};

// ── Loai vat can ────────────────────────────────────────────
enum ObsType { OBS_CACTUS_S, OBS_CACTUS_L, OBS_BIRD_LOW, OBS_BIRD_HIGH };

struct Obstacle {
    float x;
    ObsType type;
    bool active;
};

#define MAX_OBS 3

// ── Trang thai game ─────────────────────────────────────────
struct DinoGame {
    float dino_y;       // y chan khung long (GROUND_Y khi dung)
    float vel_y;        // van toc doc
    bool  jumping;
    bool  can_double;   // co the nhay doi khong
    bool  ducking;

    Obstacle obs[MAX_OBS];

    float speed;        // pixel/tick toc do dich chuyen
    long  score;
    int   hi_score;
    bool  game_over;
    bool  started;      // da bam nut chua

    int   anim_frame;   // frame chay 0/1
    unsigned long anim_last;

    // Cloud
    float cloud_x[3];
    int   cloud_y[3];

    // Ground scroll
    float ground_off;
};

static DinoGame dino;

// ── Khoi tao ─────────────────────────────────────────────────
static void Dino_Init()
{
    dino.dino_y    = GROUND_Y;
    dino.vel_y     = 0;
    dino.jumping   = false;
    dino.can_double = true;
    dino.ducking   = false;
    dino.speed     = 2.0f;
    dino.score     = 0;
    dino.game_over = false;
    dino.started   = false;
    dino.anim_frame = 0;
    dino.anim_last  = millis();
    dino.ground_off = 0;

    // Vat can
    for (int i = 0; i < MAX_OBS; i++) dino.obs[i].active = false;
    // Khoi tao vat can dau tien
    dino.obs[0].active = true;
    dino.obs[0].x      = 140;
    dino.obs[0].type   = (ObsType)random(2);

    // May
    for (int i = 0; i < 3; i++) {
        dino.cloud_x[i] = 30 + i * 45;
        dino.cloud_y[i] = 5 + random(12);
    }
}

// ── Va cham ─────────────────────────────────────────────────
static bool Dino_HitTest()
{
    // Hitbox khung long
    int dh = dino.ducking ? DINO_DUCK_H : DINO_H;
    int dy = (int)dino.dino_y - dh;
    int dx = DINO_X + 2;
    int dw = DINO_W - 4;

    for (int i = 0; i < MAX_OBS; i++) {
        if (!dino.obs[i].active) continue;
        int ox = (int)dino.obs[i].x;

        int ow, oh, oy;
        switch (dino.obs[i].type) {
            case OBS_CACTUS_S:
                ow = 8; oh = 16; oy = GROUND_Y - 16; break;
            case OBS_CACTUS_L:
                ow = 12; oh = 20; oy = GROUND_Y - 20; break;
            case OBS_BIRD_LOW:
                ow = 8; oh = 5; oy = GROUND_Y - 20; break;
            default: // OBS_BIRD_HIGH
                ow = 8; oh = 5; oy = GROUND_Y - 32; break;
        }

        // AABB
        if (dx < ox + ow - 1 && dx + dw > ox + 1 &&
            dy < oy + oh - 1 && dy + dh > oy + 1)
            return true;
    }
    return false;
}

// ── Spawn vat can moi ───────────────────────────────────────
static void Dino_SpawnNext()
{
    // Tim slot trong
    for (int i = 0; i < MAX_OBS; i++) {
        if (!dino.obs[i].active) {
            dino.obs[i].active = true;
            dino.obs[i].x      = 128 + random(OBS_MIN_GAP, OBS_MAX_GAP);
            // O level cao co chim
            if (dino.score > 300 && random(3) == 0)
                dino.obs[i].type = (random(2) == 0) ? OBS_BIRD_LOW : OBS_BIRD_HIGH;
            else
                dino.obs[i].type = (ObsType)random(2);
            break;
        }
    }
}

// ── Ve mat dat ──────────────────────────────────────────────
static void Dino_DrawGround()
{
    // Duong mat dat chinh
    display.drawFastHLine(0, GROUND_Y + 1, 128, WHITE);
    // Chi tiet mat dat (cuon)
    int off = (int)dino.ground_off % 16;
    for (int x = -off; x < 128; x += 16) {
        display.drawPixel(x + 3,  GROUND_Y + 2, WHITE);
        display.drawPixel(x + 10, GROUND_Y + 3, WHITE);
    }
}

// ── Ve may ──────────────────────────────────────────────────
static void Dino_DrawClouds()
{
    for (int i = 0; i < 3; i++) {
        int cx = (int)dino.cloud_x[i];
        int cy = dino.cloud_y[i];
        display.drawFastHLine(cx + 2, cy + 0, 4, WHITE);
        display.drawFastHLine(cx + 0, cy + 1, 8, WHITE);
        display.drawFastHLine(cx + 1, cy + 2, 6, WHITE);
    }
}

// ── Ve khung long ───────────────────────────────────────────
static void Dino_DrawDino()
{
    int dy_top = (int)dino.dino_y;

    if (dino.ducking) {
        dy_top -= DINO_DUCK_H;
        display.drawBitmap(DINO_X, dy_top, DINO_SPRITE_DUCK, 12, 8, WHITE);
    } else {
        dy_top -= DINO_H;
        const uint8_t* sp = (dino.anim_frame == 0) ? DINO_SPRITE_RUN1 : DINO_SPRITE_RUN2;
        display.drawBitmap(DINO_X, dy_top, sp, 12, 13, WHITE);
    }
}

// ── Ve vat can ──────────────────────────────────────────────
static void Dino_DrawObs()
{
    for (int i = 0; i < MAX_OBS; i++) {
        if (!dino.obs[i].active) continue;
        int ox = (int)dino.obs[i].x;
        if (ox > 128 || ox < -20) continue;

        switch (dino.obs[i].type) {
            case OBS_CACTUS_S:
                display.drawBitmap(ox, GROUND_Y - 16, CACTUS_S, 8, 16, WHITE);
                break;
            case OBS_CACTUS_L:
                display.drawBitmap(ox, GROUND_Y - 20, CACTUS_L, 12, 20, WHITE);
                break;
            case OBS_BIRD_LOW:
                if ((millis() / 200) % 2 == 0)
                    display.drawBitmap(ox, GROUND_Y - 20, BIRD_SPRITE1, 8, 5, WHITE);
                else
                    display.drawBitmap(ox, GROUND_Y - 20, BIRD_SPRITE2, 8, 5, WHITE);
                break;
            case OBS_BIRD_HIGH:
                if ((millis() / 200) % 2 == 0)
                    display.drawBitmap(ox, GROUND_Y - 32, BIRD_SPRITE1, 8, 5, WHITE);
                else
                    display.drawBitmap(ox, GROUND_Y - 32, BIRD_SPRITE2, 8, 5, WHITE);
                break;
        }
    }
}

// ── Ve HUD ──────────────────────────────────────────────────
static void Dino_DrawHUD()
{
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(70, 0);
    char buf[16];
    snprintf(buf, sizeof(buf), "HI %05ld", (long)dino.hi_score);
    display.print(buf);
    display.setCursor(70, 9);
    snprintf(buf, sizeof(buf), "   %05ld", dino.score);
    display.print(buf);
}

// ── Ham chinh ────────────────────────────────────────────────
void Game_Dino()
{
    Dino_Init();

    // Man hinh chao
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(20, 20);
    display.print("KHUNG LONG NHAY");
    display.setCursor(15, 35);
    display.print("CTR = Nhay  R = Nhay2");
    display.setCursor(20, 45);
    display.print("L = Cro / Thoat");
    display.setCursor(25, 55);
    display.print("Bam CTR de bat dau");
    display.display();

    // Cho bam nut bat dau
    while (true) {
        if (ReadButtonWait(BUTTON_CENTER)) { HaltTillRelease(BUTTON_CENTER); break; }
        if (ReadButtonWait(BUTTON_LEFT))  return; // thoat
        delay(10);
    }

    Dino_Init();
    dino.started = true;

    unsigned long last_tick = millis();

    while (true) {
        unsigned long now = millis();
        if (now - last_tick < GAME_TICK) { delay(2); continue; }
        last_tick = now;

        if (dino.game_over) {
            // Luu hi score
            if (dino.score > dino.hi_score) dino.hi_score = (int)dino.score;

            display.clearDisplay();
            display.setTextColor(WHITE);
            display.setTextSize(1);
            display.setCursor(28, 8);
            display.print("GAME OVER");
            display.setCursor(10, 22);
            display.print("Score: ");
            display.print(dino.score);
            display.setCursor(10, 32);
            display.print("Hi: ");
            display.print(dino.hi_score);
            display.setCursor(5, 45);
            display.print("CTR = Choi lai");
            display.setCursor(5, 55);
            display.print("L = Thoat menu");
            display.display();

            delay(400);
            HaltTillRelease(BUTTON_CENTER);

            while (true) {
                if (ReadButtonWait(BUTTON_CENTER)) {
                    int saved_hi = dino.hi_score;
                    Dino_Init();
                    dino.hi_score = saved_hi;
                    dino.started  = true;
                    break;
                }
                if (ReadButtonWait(BUTTON_LEFT)) return;
                delay(10);
            }
            last_tick = millis();
            continue;
        }

        // ── Xu ly nut ──────────────────────────────────────
        // CENTER = Nhay
        if (ReadButtonWait(BUTTON_CENTER)) {
            if (!dino.jumping) {
                dino.vel_y   = JUMP_VEL;
                dino.jumping = true;
                dino.can_double = true;
            } else if (dino.can_double) {
                dino.vel_y     = DOUBLE_JUMP_VEL;
                dino.can_double = false;
            }
            HaltTillRelease(BUTTON_CENTER);
        }

        // RIGHT = Nhay doi
        if (ReadButtonWait(BUTTON_RIGHT)) {
            if (!dino.jumping) {
                dino.vel_y   = JUMP_VEL;
                dino.jumping = true;
                dino.can_double = true;
            } else if (dino.can_double) {
                dino.vel_y     = DOUBLE_JUMP_VEL;
                dino.can_double = false;
            }
        }

        // LEFT giu = Cro; LEFT tap (khi dang chay man hinh game) = thoat
        static bool left_held = false;
        static unsigned long left_t = 0;
        if (ReadButton(BUTTON_LEFT)) {
            if (!left_held) { left_held = true; left_t = now; }
            // Giu >200ms = cro
            if (now - left_t > 200) dino.ducking = true;
        } else {
            if (left_held) {
                if (now - left_t < 200) {
                    // Tap nhe = thoat
                    return;
                }
                dino.ducking = false;
            }
            left_held = false;
        }

        // Khong cro neu dang nhay
        if (dino.jumping) dino.ducking = false;

        // ── Vat ly khung long ──────────────────────────────
        dino.vel_y  += GRAVITY;
        dino.dino_y += dino.vel_y;

        if (dino.dino_y >= GROUND_Y) {
            dino.dino_y    = GROUND_Y;
            dino.vel_y     = 0;
            dino.jumping   = false;
            dino.can_double = true;
        }

        // ── Di chuyen vat can ──────────────────────────────
        bool need_spawn = false;
        for (int i = 0; i < MAX_OBS; i++) {
            if (!dino.obs[i].active) continue;
            dino.obs[i].x -= dino.speed;
            if (dino.obs[i].x < -20) {
                dino.obs[i].active = false;
                need_spawn = true;
            }
        }
        if (need_spawn) Dino_SpawnNext();

        // ── Di chuyen may ──────────────────────────────────
        for (int i = 0; i < 3; i++) {
            dino.cloud_x[i] -= dino.speed * 0.4f;
            if (dino.cloud_x[i] < -20) {
                dino.cloud_x[i] = 128 + random(10, 40);
                dino.cloud_y[i] = 3 + random(15);
            }
        }

        // ── Cuon mat dat ───────────────────────────────────
        dino.ground_off += dino.speed;

        // ── Diem & tang toc ────────────────────────────────
        dino.score++;
        if (dino.score % 100 == 0) {
            dino.speed = min(7.0f, dino.speed + 0.2f);
        }

        // ── Animation khung long ───────────────────────────
        if (now - dino.anim_last > 120) {
            dino.anim_frame ^= 1;
            dino.anim_last = now;
        }

        // ── Kiem tra va cham ───────────────────────────────
        if (Dino_HitTest()) {
            dino.game_over = true;
        }

        // ── Ve ─────────────────────────────────────────────
        display.clearDisplay();
        Dino_DrawClouds();
        Dino_DrawGround();
        Dino_DrawObs();
        Dino_DrawDino();
        Dino_DrawHUD();
        display.display();
    }
}
