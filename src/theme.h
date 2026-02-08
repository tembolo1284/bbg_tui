/*
** theme.h — Bloomberg Terminal Color Palette
**
** Centralized color definitions for the bbg_tui project.
** All colors approximate the Bloomberg Professional terminal aesthetic.
*/

#ifndef THEME_H
#define THEME_H

#include "bbg_tui.h"

/* ---- Background Tones ---- */
#define TH_BG               mu_color(  1,   2,   8, 255)
#define TH_WINDOW_BG        mu_color( 16,  18,  26, 255)
#define TH_PANEL_BG         mu_color( 12,  14,  22, 255)
#define TH_HEADER_BG        mu_color( 30,  33,  46, 255)
#define TH_ROW_EVEN         mu_color( 14,  16,  24, 255)
#define TH_ROW_ODD          mu_color( 18,  20,  30, 255)
#define TH_ROW_SELECTED     mu_color( 35,  40,  65, 255)
#define TH_ROW_HOVER        mu_color( 28,  32,  50, 255)
#define TH_SEPARATOR        mu_color( 40,  44,  58, 255)
#define TH_TITLE_BG         mu_color( 30,  33,  46, 255)
#define TH_BORDER            mu_color( 45,  50,  68, 255)
#define TH_GROUP_BG         mu_color( 22,  25,  38, 255)
#define TH_SUMMARY_BG       mu_color( 25,  28,  42, 255)
#define TH_STATUS_BG        mu_color( 10,  12,  18, 255)

/* ---- Tab Bar ---- */
#define TH_TAB_BG           mu_color( 20,  22,  32, 255)
#define TH_TAB_ACTIVE       mu_color( 40,  44,  65, 255)
#define TH_TAB_HOVER        mu_color( 30,  34,  50, 255)
#define TH_TAB_TEXT          mu_color(160, 160, 170, 255)
#define TH_TAB_TEXT_ACTIVE   mu_color(255, 170,  40, 255)

/* ---- Text Tones ---- */
#define TH_TEXT             mu_color(200, 200, 210, 255)
#define TH_TEXT_DIM         mu_color(120, 125, 140, 255)
#define TH_TEXT_BRIGHT      mu_color(240, 240, 248, 255)
#define TH_HEADER_TEXT      mu_color(255, 170,  40, 255)   /* bloomberg amber */
#define TH_TITLE_TEXT       mu_color(255, 140,   0, 255)

/* ---- Semantic Colors ---- */
#define TH_PNL_POS          mu_color( 60, 220,  80, 255)
#define TH_PNL_NEG          mu_color(255,  60,  60, 255)
#define TH_PNL_ZERO         mu_color(140, 140, 150, 255)
#define TH_STALE            mu_color(200, 180,  40, 255)
#define TH_BUTTON           mu_color( 50,  55,  75, 255)
#define TH_BUTTON_HOVER     mu_color( 65,  70,  95, 255)
#define TH_BUTTON_ACTIVE    mu_color( 80,  85, 115, 255)

/* ---- Apply theme to a mu_Context ---- */
static inline void theme_apply(mu_Context *ctx) {
  ctx->style->colors[MU_COLOR_TEXT]        = TH_TEXT;
  ctx->style->colors[MU_COLOR_BORDER]      = TH_BORDER;
  ctx->style->colors[MU_COLOR_WINDOWBG]    = TH_WINDOW_BG;
  ctx->style->colors[MU_COLOR_TITLEBG]     = TH_TITLE_BG;
  ctx->style->colors[MU_COLOR_TITLETEXT]   = TH_TITLE_TEXT;
  ctx->style->colors[MU_COLOR_PANELBG]     = TH_PANEL_BG;
  ctx->style->colors[MU_COLOR_BUTTON]      = TH_BUTTON;
  ctx->style->colors[MU_COLOR_BUTTONHOVER] = TH_BUTTON_HOVER;
  ctx->style->colors[MU_COLOR_BUTTONFOCUS] = TH_BUTTON_ACTIVE;
  ctx->style->colors[MU_COLOR_BASE]        = mu_color(20, 22, 32, 255);
  ctx->style->colors[MU_COLOR_BASEHOVER]   = mu_color(28, 30, 42, 255);
  ctx->style->colors[MU_COLOR_BASEFOCUS]   = mu_color(35, 38, 55, 255);
  ctx->style->colors[MU_COLOR_SCROLLBASE]  = mu_color(20, 22, 30, 255);
  ctx->style->colors[MU_COLOR_SCROLLTHUMB] = mu_color(55, 60, 80, 255);
  ctx->style->padding       = 3;
  ctx->style->spacing       = 1;
  ctx->style->indent        = 18;
  ctx->style->title_height  = 22;
  ctx->style->scrollbar_size = 10;
  ctx->style->thumb_size    = 6;
}

/* ---- Helpers ---- */
static inline mu_Color th_pnl_color(double val) {
  return (val >  0.01) ? TH_PNL_POS
       : (val < -0.01) ? TH_PNL_NEG
       :                  TH_PNL_ZERO;
}

#endif
