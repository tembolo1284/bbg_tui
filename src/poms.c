/*
** poms.c — POMS Position Grid Rendering
*/

#include <stdio.h>
#include <string.h>
#include "poms.h"
#include "table.h"
#include "theme.h"

/* ---- Column Layout ---- */

#define COL_COUNT 15

static const int COL_W[COL_COUNT] = {
  155,  /* 0  Instrument      */
   88,  /* 1  CUSIP/ISIN      */
   48,  /* 2  Book            */
   48,  /* 3  Desk            */
   62,  /* 4  Notional (MM)   */
   58,  /* 5  Avg Price       */
   58,  /* 6  Mkt Price       */
   62,  /* 7  Total P&L (K)   */
   55,  /* 8  Day P&L (K)     */
   52,  /* 9  DV01            */
   45,  /* 10 CS01            */
   45,  /* 11 Delta           */
   48,  /* 12 Vega            */
   48,  /* 13 Theta           */
   45,  /* 14 Gamma           */
};

static const char *COL_HDR[COL_COUNT] = {
  "INSTRUMENT", "CUSIP/ISIN", "BOOK", "DESK", "NOTL(MM)",
  "AVG PX", "MKT PX", "P&L(K)", "DAY P&L",
  "DV01", "CS01", "DELTA", "VEGA", "THETA", "GAMMA"
};

#define ROW_H 18

/* ---- Abbreviate book names to fit column ---- */
static const char *book_short(const char *book) {
  if (strcmp(book, "Rates Flow") == 0) return "FLOW";
  if (strcmp(book, "Swaps")      == 0) return "SWAP";
  if (strcmp(book, "Futures")    == 0) return "FUT";
  if (strcmp(book, "Vol Desk")   == 0) return "VOL";
  return book;
}

static const char *desk_short(const char *desk) {
  if (strcmp(desk, "US Rates")  == 0) return "USR";
  if (strcmp(desk, "EUR Rates") == 0) return "EUR";
  if (strcmp(desk, "GBP Rates") == 0) return "GBP";
  if (strcmp(desk, "USD Swaps") == 0) return "USDSW";
  if (strcmp(desk, "EUR Swaps") == 0) return "EURSW";
  if (strcmp(desk, "GBP Swaps") == 0) return "GBPSW";
  if (strcmp(desk, "USD Vol")   == 0) return "USDVL";
  if (strcmp(desk, "EUR Vol")   == 0) return "EURVL";
  return desk;
}


/* ---- Header Row ---- */

static void draw_header(mu_Context *ctx) {
  mu_layout_row(ctx, COL_COUNT, COL_W, ROW_H);
  for (int c = 0; c < COL_COUNT; c++) {
    mu_Rect r = mu_layout_next(ctx);
    mu_draw_rect(ctx, r, TH_HEADER_BG);

    mu_Font font = ctx->style->font;
    int tw = ctx->text_width(font, COL_HDR[c], -1);
    int th = ctx->text_height(font);
    mu_Vec2 pos;
    pos.y = r.y + (r.h - th) / 2;
    pos.x = (c >= 4) ? r.x + r.w - tw - 2 : r.x + 2;  /* numeric cols right-align */

    mu_push_clip_rect(ctx, r);
    mu_draw_text(ctx, font, COL_HDR[c], -1, pos, TH_HEADER_TEXT);
    mu_pop_clip_rect(ctx);
    tbl_separator(ctx, r, TH_SEPARATOR);
  }
}


/* ---- Position Row ---- */

/* GCC -Wformat-truncation=2 warns that a double *could* produce 300+ bytes.
   Our values are bounded financial data and snprintf truncates safely.
   These are display-only cell strings — truncation is harmless. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"

static void draw_row(mu_Context *ctx, const Position *p, int row_idx, int tick) {
  char buf[64];
  mu_Color bg = (row_idx % 2 == 0) ? TH_ROW_EVEN : TH_ROW_ODD;
  mu_Color txt = p->stale ? TH_STALE : TH_TEXT;

  mu_layout_row(ctx, COL_COUNT, COL_W, ROW_H);

  /* 0: Instrument — with stale dot */
  {
    mu_Rect r = mu_layout_next(ctx);
    mu_draw_rect(ctx, r, bg);
    int xoff = 2;
    if (p->stale && (tick / 30) % 2) {
      mu_draw_rect(ctx, mu_rect(r.x + 1, r.y + r.h/2 - 2, 4, 4), TH_STALE);
      xoff = 7;
    }
    mu_push_clip_rect(ctx, r);
    mu_draw_text(ctx, ctx->style->font, p->instrument, -1,
                 mu_vec2(r.x + xoff, r.y + (r.h - ctx->text_height(ctx->style->font))/2), txt);
    mu_pop_clip_rect(ctx);
    tbl_separator(ctx, r, TH_SEPARATOR);
  }

  /* 1: CUSIP */
  tbl_cell(ctx, p->cusip, bg, TH_TEXT_DIM, 0);

  /* 2: Book */
  tbl_cell(ctx, book_short(p->book), bg, TH_TEXT_DIM, 0);

  /* 3: Desk */
  tbl_cell(ctx, desk_short(p->desk), bg, TH_TEXT_DIM, 0);

  /* 4: Notional */
  snprintf(buf, sizeof(buf), "%.1f", p->notional);
  tbl_cell(ctx, buf, bg, (p->notional >= 0) ? TH_TEXT : TH_PNL_NEG, MU_OPT_ALIGNRIGHT);

  /* 5: Avg Price */
  if (p->avg_price > 0.001)
    snprintf(buf, sizeof(buf), "%.3f", p->avg_price);
  else
    snprintf(buf, sizeof(buf), "-");
  tbl_cell(ctx, buf, bg, TH_TEXT, MU_OPT_ALIGNRIGHT);

  /* 6: Mkt Price */
  if (p->mkt_price > 0.001)
    snprintf(buf, sizeof(buf), "%.3f", p->mkt_price);
  else
    snprintf(buf, sizeof(buf), "-");
  tbl_cell(ctx, buf, bg, p->stale ? TH_STALE : TH_TEXT_BRIGHT, MU_OPT_ALIGNRIGHT);

  /* 7: Total P&L */
  tbl_cell_pnl(ctx, p->pnl_total, "%+.1f", bg);

  /* 8: Day P&L */
  tbl_cell_pnl(ctx, p->pnl_day, "%+.1f", bg);

  /* 9: DV01 */
  tbl_cell_num(ctx, p->dv01, "%.0f", bg, TH_TEXT);

  /* 10: CS01 */
  tbl_cell_num(ctx, p->cs01, "%.0f", bg, (p->cs01 > 0) ? TH_TEXT : TH_TEXT_DIM);

  /* 11: Delta */
  snprintf(buf, sizeof(buf), "%.2f", p->delta);
  tbl_cell(ctx, buf, bg, TH_TEXT, MU_OPT_ALIGNRIGHT);

  /* 12: Vega */
  tbl_cell_num(ctx, p->vega, "%.1f", bg, (p->vega > 0.01) ? TH_TEXT : TH_TEXT_DIM);

  /* 13: Theta */
  tbl_cell_pnl(ctx, p->theta, "%.2f", bg);

  /* 14: Gamma */
  snprintf(buf, sizeof(buf), "%.3f", p->gamma);
  tbl_cell(ctx, buf, bg, TH_TEXT_DIM, MU_OPT_ALIGNRIGHT);
}

#pragma GCC diagnostic pop


/* ---- Book Group Header ---- */

static void draw_group_header(mu_Context *ctx, const char *book, const char *desk) {
  mu_layout_row(ctx, 1, (int[]){ -1 }, ROW_H);
  mu_Rect r = mu_layout_next(ctx);
  mu_draw_rect(ctx, r, TH_GROUP_BG);

  char hdr[64];
  snprintf(hdr, sizeof(hdr), ">> %s / %s", book, desk);
  mu_push_clip_rect(ctx, r);
  mu_draw_text(ctx, ctx->style->font, hdr, -1,
               mu_vec2(r.x + 4, r.y + (r.h - ctx->text_height(ctx->style->font))/2),
               TH_HEADER_TEXT);
  mu_pop_clip_rect(ctx);
}


/* ---- Summary / Totals ---- */

static void draw_summary(mu_Context *ctx, PositionBook *book, ScreenFilter *flt) {
  double tot_notl = 0, tot_pnl = 0, tot_dpnl = 0;
  double tot_dv01 = 0, tot_cs01 = 0, tot_vega = 0, tot_theta = 0;
  int count = 0;

  for (int i = 0; i < book->count; i++) {
    if (!screen_filter_check(flt, &book->items[i])) continue;
    const Position *p = &book->items[i];
    tot_notl  += p->notional;
    tot_pnl   += p->pnl_total;
    tot_dpnl  += p->pnl_day;
    tot_dv01  += p->dv01;
    tot_cs01  += p->cs01;
    tot_vega  += p->vega;
    tot_theta += p->theta;
    count++;
  }

  mu_Color bg = TH_SUMMARY_BG;
  char buf[64];

  mu_layout_row(ctx, COL_COUNT, COL_W, ROW_H + 2);

  /* TOTAL label */
  snprintf(buf, sizeof(buf), "TOTAL (%d)", count);
  tbl_cell(ctx, buf, bg, TH_HEADER_TEXT, 0);

  /* CUSIP, Book, Desk — empty */
  tbl_cell_empty(ctx, bg);
  tbl_cell_empty(ctx, bg);
  tbl_cell_empty(ctx, bg);

  /* Notional */
  tbl_cell_pnl(ctx, tot_notl, "%.1f", bg);

  /* Avg/Mkt — not meaningful */
  tbl_cell_empty(ctx, bg);
  tbl_cell_empty(ctx, bg);

  /* P&L totals */
  tbl_cell_pnl(ctx, tot_pnl, "%+.1f", bg);
  tbl_cell_pnl(ctx, tot_dpnl, "%+.1f", bg);

  /* Risk totals */
  tbl_cell_num(ctx, tot_dv01, "%.0f", bg, TH_TEXT_BRIGHT);
  tbl_cell_num(ctx, tot_cs01, "%.0f", bg, TH_TEXT);

  /* Delta — skip aggregate */
  tbl_cell_empty(ctx, bg);

  tbl_cell_num(ctx, tot_vega, "%.1f", bg, TH_TEXT);
  tbl_cell_pnl(ctx, tot_theta, "%.2f", bg);

  /* Gamma — skip aggregate */
  tbl_cell_empty(ctx, bg);
}


/* ---- Status Bar ---- */

static void draw_status(mu_Context *ctx, int n_positions) {
  mu_layout_row(ctx, 1, (int[]){ -1 }, 16);
  mu_Rect r = mu_layout_next(ctx);
  mu_draw_rect(ctx, r, TH_STATUS_BG);

  char buf[128];
  snprintf(buf, sizeof(buf), " bbg_tui v0.1 | Positions: %d | engine: microui %s",
           n_positions, MU_VERSION);
  mu_push_clip_rect(ctx, r);
  mu_draw_text(ctx, ctx->style->font, buf, -1, mu_vec2(r.x + 2, r.y + 1), TH_TEXT_DIM);
  mu_pop_clip_rect(ctx);
}


/* ============================================================================
**  Main Render Entry Point
** ============================================================================*/

void poms_render(mu_Context *ctx, Screen *scr, PositionBook *book, int tick) {
  ScreenFilter *flt = &scr->filter;

  /* ---- Filter Controls ---- */
  mu_layout_row(ctx, 7, (int[]){ 50, 120, 55, 55, 55, 55, -1 }, 22);
  mu_label(ctx, "Filter:");
  mu_textbox(ctx, flt->search, sizeof(flt->search));
  mu_checkbox(ctx, "Bond", &flt->show_bonds);
  mu_checkbox(ctx, "Swap", &flt->show_swaps);
  mu_checkbox(ctx, "Fut",  &flt->show_futures);
  mu_checkbox(ctx, "Vol",  &flt->show_swaptions);
  mu_layout_next(ctx); /* spacer */

  /* ---- Separator ---- */
  mu_layout_row(ctx, 1, (int[]){ -1 }, 1);
  mu_draw_rect(ctx, mu_layout_next(ctx), TH_SEPARATOR);

  /* ---- Column Headers ---- */
  draw_header(ctx);

  mu_layout_row(ctx, 1, (int[]){ -1 }, 1);
  mu_draw_rect(ctx, mu_layout_next(ctx), TH_SEPARATOR);

  /* ---- Scrollable Grid ---- */
  mu_layout_row(ctx, 1, (int[]){ -1 }, -42);
  mu_begin_panel(ctx, "grid");
  {
    int row_idx = 0;
    const char *last_book = NULL;

    for (int i = 0; i < book->count; i++) {
      Position *p = &book->items[i];
      if (!screen_filter_check(flt, p)) continue;

      /* Book group separator */
      if (!last_book || strcmp(last_book, p->book) != 0) {
        if (last_book) {
          mu_layout_row(ctx, 1, (int[]){ -1 }, 2);
          mu_draw_rect(ctx, mu_layout_next(ctx), TH_SEPARATOR);
        }
        draw_group_header(ctx, p->book, p->desk);
        last_book = p->book;
      }

      draw_row(ctx, p, row_idx, tick);
      row_idx++;
    }
  }
  mu_end_panel(ctx);

  /* ---- Amber accent line above totals ---- */
  mu_layout_row(ctx, 1, (int[]){ -1 }, 1);
  mu_draw_rect(ctx, mu_layout_next(ctx), TH_HEADER_TEXT);

  /* ---- Summary ---- */
  draw_summary(ctx, book, flt);

  /* ---- Status ---- */
  draw_status(ctx, book->count);
}
