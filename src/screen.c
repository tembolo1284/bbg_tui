/*
** screen.c — Multi-Screen Manager
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "screen.h"
#include "theme.h"


void screen_mgr_init(ScreenManager *mgr) {
  memset(mgr, 0, sizeof(*mgr));
  mgr->next_id = 1;

  /* Default screen: show everything */
  screen_mgr_add(mgr, "ALL");
}


int screen_mgr_add(ScreenManager *mgr, const char *name) {
  return screen_mgr_add_preset(mgr, name, 1, 1, 1, 1);
}


int screen_mgr_add_preset(ScreenManager *mgr, const char *name,
                            int bonds, int swaps, int futures, int swaptions)
{
  if (mgr->count >= MAX_SCREENS) return -1;

  int idx = mgr->count;
  Screen *s = &mgr->screens[idx];
  memset(s, 0, sizeof(*s));

  if (name) {
    snprintf(s->name, SCREEN_NAME_LEN, "%s", name);
  } else {
    snprintf(s->name, SCREEN_NAME_LEN, "POMS %d", mgr->next_id);
  }
  mgr->next_id++;

  s->filter.show_bonds     = bonds;
  s->filter.show_swaps     = swaps;
  s->filter.show_futures   = futures;
  s->filter.show_swaptions = swaptions;
  s->selected_row          = -1;
  s->active                = 1;

  mgr->count++;
  mgr->active_idx = idx;
  return idx;
}


void screen_mgr_remove(ScreenManager *mgr, int idx) {
  if (mgr->count <= 1) return;  /* always keep at least one */
  if (idx < 0 || idx >= mgr->count) return;

  /* shift down */
  for (int i = idx; i < mgr->count - 1; i++) {
    mgr->screens[i] = mgr->screens[i + 1];
  }
  mgr->count--;

  if (mgr->active_idx >= mgr->count) {
    mgr->active_idx = mgr->count - 1;
  } else if (mgr->active_idx > idx) {
    mgr->active_idx--;
  }
}


Screen* screen_mgr_active(ScreenManager *mgr) {
  return &mgr->screens[mgr->active_idx];
}


int screen_mgr_tab_bar(ScreenManager *mgr, mu_Context *ctx) {
  int changed = 0;
  int tab_w = 80;
  int tab_h = 20;
  int add_w = 25;

  /* Calculate widths array: one per tab + [+] button + spacer */
  int n = mgr->count;
  int widths[MAX_SCREENS + 2];
  for (int i = 0; i < n; i++) widths[i] = tab_w;
  widths[n] = add_w;       /* [+] button */
  widths[n + 1] = -1;      /* spacer fills rest */

  mu_layout_row(ctx, n + 2, widths, tab_h);

  for (int i = 0; i < n; i++) {
    mu_Rect r = mu_layout_next(ctx);
    int is_active = (i == mgr->active_idx);

    /* background */
    mu_Color bg = is_active ? TH_TAB_ACTIVE : TH_TAB_BG;

    /* hover detection using mu_get_id + mu_update_control */
    char id_buf[32];
    snprintf(id_buf, sizeof(id_buf), "!tab_%d", i);
    mu_Id id = mu_get_id(ctx, id_buf, (int)strlen(id_buf));
    mu_update_control(ctx, id, r, 0);

    if (ctx->hover == id && !is_active) bg = TH_TAB_HOVER;

    mu_draw_rect(ctx, r, bg);

    /* tab text */
    mu_Color fg = is_active ? TH_TAB_TEXT_ACTIVE : TH_TAB_TEXT;
    mu_Font font = ctx->style->font;
    int tw = ctx->text_width(font, mgr->screens[i].name, -1);
    int th = ctx->text_height(font);
    mu_push_clip_rect(ctx, r);
    mu_draw_text(ctx, font, mgr->screens[i].name, -1,
                 mu_vec2(r.x + (r.w - tw) / 2, r.y + (r.h - th) / 2), fg);
    mu_pop_clip_rect(ctx);

    /* click to switch */
    if (ctx->mouse_pressed == MU_MOUSE_LEFT && ctx->focus == id) {
      mgr->active_idx = i;
      changed = 1;
    }

    /* right-click to close (if not the last screen) */
    if (ctx->mouse_pressed == MU_MOUSE_RIGHT && ctx->focus == id && n > 1) {
      screen_mgr_remove(mgr, i);
      changed = 1;
      break;  /* layout is now invalid, will redraw next frame */
    }

    /* bottom border: highlight active tab */
    if (is_active) {
      mu_draw_rect(ctx, mu_rect(r.x, r.y + r.h - 2, r.w, 2), TH_HEADER_TEXT);
    }

    /* right edge separator between tabs */
    mu_draw_rect(ctx, mu_rect(r.x + r.w - 1, r.y + 2, 1, r.h - 4), TH_SEPARATOR);
  }

  /* [+] Add Screen button */
  {
    mu_Rect r = mu_layout_next(ctx);
    char id_buf[] = "!tab_add";
    mu_Id id = mu_get_id(ctx, id_buf, (int)strlen(id_buf));
    mu_update_control(ctx, id, r, 0);

    mu_Color bg = (ctx->hover == id) ? TH_BUTTON_HOVER : TH_BUTTON;
    mu_draw_rect(ctx, r, bg);

    mu_Font font = ctx->style->font;
    int tw = ctx->text_width(font, "+", -1);
    int th = ctx->text_height(font);
    mu_push_clip_rect(ctx, r);
    mu_draw_text(ctx, font, "+", -1,
                 mu_vec2(r.x + (r.w - tw) / 2, r.y + (r.h - th) / 2),
                 TH_TEXT_BRIGHT);
    mu_pop_clip_rect(ctx);

    if (ctx->mouse_pressed == MU_MOUSE_LEFT && ctx->focus == id) {
      if (mgr->count < MAX_SCREENS) {
        screen_mgr_add(mgr, NULL);
        changed = 1;
      }
    }
  }

  /* spacer */
  mu_layout_next(ctx);

  return changed;
}


/* ---- Filter matching ---- */

static int str_contains_ci(const char *haystack, const char *needle) {
  if (!needle[0]) return 1;
  int nlen = (int)strlen(needle);
  int hlen = (int)strlen(haystack);
  if (nlen > hlen) return 0;

  for (int i = 0; i <= hlen - nlen; i++) {
    int match = 1;
    for (int j = 0; j < nlen && match; j++) {
      char a = haystack[i + j];
      char b = needle[j];
      if (a >= 'A' && a <= 'Z') a += 32;
      if (b >= 'A' && b <= 'Z') b += 32;
      if (a != b) match = 0;
    }
    if (match) return 1;
  }
  return 0;
}


int screen_filter_check(const ScreenFilter *f, const Position *p) {
  /* asset class filter */
  switch (p->asset_class) {
    case ASSET_GOVT_BOND: if (!f->show_bonds) return 0; break;
    case ASSET_IRS:
    case ASSET_FRA:       if (!f->show_swaps) return 0; break;
    case ASSET_FUTURES:   if (!f->show_futures) return 0; break;
    case ASSET_SWAPTION:  if (!f->show_swaptions) return 0; break;
    default: break;
  }

  /* text search across instrument, cusip, book, desk */
  if (f->search[0]) {
    if (str_contains_ci(p->instrument, f->search)) return 1;
    if (str_contains_ci(p->cusip, f->search)) return 1;
    if (str_contains_ci(p->book, f->search)) return 1;
    if (str_contains_ci(p->desk, f->search)) return 1;
    return 0;
  }

  return 1;
}
