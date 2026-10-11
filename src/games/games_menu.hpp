#pragma once
/*
 * ============================================================
 *  GAMES_MENU.HPP  —  Menu cac tro choi
 *  Cac game deu choi bang 3 nut LEFT / CENTER / RIGHT
 * ============================================================
 *
 *  Nut chung khi dang trong game:
 *    LEFT   = Xoay / Cro / Thoat (pha thuoc game)
 *    CENTER = Chon / Nhay / Drop
 *    RIGHT  = Di chuyen / Nhay doi
 *
 *  De thoat game → vao lai menu: giu/nhan LEFT
 * ============================================================
 */

#include "tetris.hpp"
#include "dino.hpp"

// Ham nay goi tu main.cpp
// Menu games duoc dung nhu lambda callback trong menu chinh:
//   menu_games.AddItem(MenuItem("Xep Gach", Game_Tetris));
//   menu_games.AddItem(MenuItem("Khung Long", Game_Dino));
