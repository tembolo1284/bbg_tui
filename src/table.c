/*
** table.c — Table cell rendering helpers
*/

#include <stdio.h>
#include <string.h>
#include "table.h"
#include "theme.h"

void tbl_separator(mu_Context *ctx, mu_Rect r, mu_Color color) {
  mu_draw_rect(ctx, mu_rect(r.x + r.w - 1, r.y, 1, r.h), color);
}


void tbl_cell(mu_Context *ctx, const char *text, mu_Color bg, mu_Color fg, int opt) {
  mu_Rect r = mu_layout_next(ctx);
  mu_draw_rect(ctx, r, bg);

  mu_Font font = ctx->style->font;
  int tw = ctx->text_width(font, text, -1);
  int th = ctx->text_height(font);

  mu_Vec2 pos;
  pos.y = r.y + (r.h - th) / 2;
  if (opt & MU_OPT_ALIGNRIGHT) {
    pos.x = r.x + r.w - tw - 2;
  } else if (opt & MU_OPT_ALIGNCENTER) {
    pos.x = r.x + (r.w - tw) / 2;
  } else {
    pos.x = r.x + 2;
  }

  mu_push_clip_rect(ctx, r);
  mu_draw_text(ctx, font, text, -1, pos, fg);
  mu_pop_clip_rect(ctx);
  tbl_separator(ctx, r, TH_SEPARATOR);
}


void tbl_cell_text(mu_Context *ctx, const char *text, mu_Color fg, int opt) {
  /* transparent background — caller is responsible for row bg */
  mu_Rect r = mu_layout_next(ctx);

  mu_Font font = ctx->style->font;
  int tw = ctx->text_width(font, text, -1);
  int th = ctx->text_height(font);

  mu_Vec2 pos;
  pos.y = r.y + (r.h - th) / 2;
  if (opt & MU_OPT_ALIGNRIGHT) {
    pos.x = r.x + r.w - tw - 2;
  } else if (opt & MU_OPT_ALIGNCENTER) {
    pos.x = r.x + (r.w - tw) / 2;
  } else {
    pos.x = r.x + 2;
  }

  mu_push_clip_rect(ctx, r);
  mu_draw_text(ctx, font, text, -1, pos, fg);
  mu_pop_clip_rect(ctx);
  tbl_separator(ctx, r, TH_SEPARATOR);
}


/* fmt is always a compile-time literal from our call sites in poms.c,
   but gcc can't prove that through a function parameter. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"

void tbl_cell_pnl(mu_Context *ctx, double val, const char *fmt,
                   mu_Color bg)
{
  char buf[32];
  snprintf(buf, sizeof(buf), fmt, val);
  mu_Color fg = th_pnl_color(val);
  tbl_cell(ctx, buf, bg, fg, MU_OPT_ALIGNRIGHT);
}


void tbl_cell_num(mu_Context *ctx, double val, const char *fmt,
                  mu_Color bg, mu_Color fg)
{
  char buf[32];
  snprintf(buf, sizeof(buf), fmt, val);
  tbl_cell(ctx, buf, bg, fg, MU_OPT_ALIGNRIGHT);
}

#pragma GCC diagnostic pop


void tbl_cell_empty(mu_Context *ctx, mu_Color bg) {
  mu_Rect r = mu_layout_next(ctx);
  mu_draw_rect(ctx, r, bg);
  tbl_separator(ctx, r, TH_SEPARATOR);
}
