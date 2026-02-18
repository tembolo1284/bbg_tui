/*
** screen.c — Multi-Screen Manager
**
** Tab features:
**   Click          → switch to screen
**   Double-click   → rename screen (inline textbox)
**   Right-click    → close confirmation popup
**   Drag-and-drop  → reorder tabs
**   [+] button     → add new screen
**   Ctrl+T         → add new screen (handled in main.c)
**   Enter/Esc      → commit/cancel rename
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "screen.h"
#include "theme.h"

/* ---- Double-click detection ---- */
#define DBLCLICK_FRAMES 18  /* ~300ms at 60fps */


void screen_mgr_init(ScreenManager *mgr) {
  memset(mgr, 0, sizeof(*mgr));
  mgr->next_id           = 1;
  mgr->rename_idx        = -1;
  mgr->close_confirm_idx = -1;
  mgr->drag_idx          = -1;
  mgr->drag_target       = -1;

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
  if (mgr->count <= 1) return;
  if (idx < 0 || idx >= mgr->count) return;

  /* Cancel rename if removing the tab being renamed */
  if (mgr->rename_idx == idx) mgr->rename_idx = -1;
  else if (mgr->rename_idx > idx) mgr->rename_idx--;

  /* Adjust close_confirm_idx */
  if (mgr->close_confirm_idx == idx) mgr->close_confirm_idx = -1;
  else if (mgr->close_confirm_idx > idx) mgr->close_confirm_idx--;

  for (int i = idx; i < mgr->count - 1; i++) {
    mgr->screens[i] = mgr->screens[i + 1];
  }
  mgr->count--;

  if (mgr->active_idx >= mgr->count)
    mgr->active_idx = mgr->count - 1;
  else if (mgr->active_idx > idx)
    mgr->active_idx--;
}


void screen_mgr_swap(ScreenManager *mgr, int a, int b) {
  if (a < 0 || a >= mgr->count) return;
  if (b < 0 || b >= mgr->count) return;
  if (a == b) return;

  Screen tmp = mgr->screens[a];
  mgr->screens[a] = mgr->screens[b];
  mgr->screens[b] = tmp;

  /* Update active_idx to follow the active screen */
  if (mgr->active_idx == a) mgr->active_idx = b;
  else if (mgr->active_idx == b) mgr->active_idx = a;

  /* Update rename_idx to follow */
  if (mgr->rename_idx == a) mgr->rename_idx = b;
  else if (mgr->rename_idx == b) mgr->rename_idx = a;
}


Screen* screen_mgr_active(ScreenManager *mgr) {
  return &mgr->screens[mgr->active_idx];
}


/* ============================================================================
**  Tab Bar Rendering
** ============================================================================*/

/* State for double-click detection (file-scoped) */
static int  s_last_click_tab   = -1;
static int  s_last_click_frame = 0;

/* Drag threshold in pixels before drag activates */
#define DRAG_THRESHOLD 6

int screen_mgr_tab_bar(ScreenManager *mgr, mu_Context *ctx) {
  int changed = 0;
  int tab_w = 80;
  int tab_h = 20;
  int add_w = 25;
  int n = mgr->count;

  /* Column widths: tabs + [+] + spacer */
  int widths[MAX_SCREENS + 2];
  for (int i = 0; i < n; i++) widths[i] = tab_w;
  widths[n] = add_w;
  widths[n + 1] = -1;

  mu_layout_row(ctx, n + 2, widths, tab_h);

  /* Track tab rects for drag-and-drop hit testing */
  mu_Rect tab_rects[MAX_SCREENS];

  for (int i = 0; i < n; i++) {
    mu_Rect r = mu_layout_next(ctx);
    tab_rects[i] = r;
    int is_active = (i == mgr->active_idx);
    int is_dragged = (mgr->dragging && mgr->drag_idx == i);

    /* ---- Background ---- */
    mu_Color bg = is_active ? TH_TAB_ACTIVE : TH_TAB_BG;

    char id_buf[32];
    snprintf(id_buf, sizeof(id_buf), "!tab_%d", i);
    mu_Id id = mu_get_id(ctx, id_buf, (int)strlen(id_buf));
    mu_update_control(ctx, id, r, 0);

    if (ctx->hover == id && !is_active && !is_dragged) bg = TH_TAB_HOVER;
    if (is_dragged) bg = TH_BUTTON_ACTIVE;

    mu_draw_rect(ctx, r, bg);

    /* ---- Rename mode: show textbox instead of label ---- */
    if (mgr->rename_idx == i) {
      mu_Rect tb = mu_rect(r.x + 2, r.y + 2, r.w - 4, r.h - 4);
      char rename_id[] = "!tab_rename";
      mu_Id rid = mu_get_id(ctx, rename_id, (int)strlen(rename_id));

      /* Force focus on first frame so user can type immediately */
      if (ctx->focus != rid) {
        mu_set_focus(ctx, rid);
      }

      int res = mu_textbox_raw(ctx, mgr->rename_buf, SCREEN_NAME_LEN, rid, tb,
                               MU_OPT_HOLDFOCUS);

      /* Commit on Enter */
      if (res & MU_RES_SUBMIT) {
        if (mgr->rename_buf[0]) {
          snprintf(mgr->screens[i].name, SCREEN_NAME_LEN, "%s", mgr->rename_buf);
        }
        mgr->rename_idx = -1;
      }

      /* Cancel if focus lost (clicked elsewhere) */
      if (ctx->focus != rid && !(res & MU_RES_ACTIVE)) {
        if (mgr->rename_buf[0]) {
          snprintf(mgr->screens[i].name, SCREEN_NAME_LEN, "%s", mgr->rename_buf);
        }
        mgr->rename_idx = -1;
      }
    } else {
      /* ---- Normal tab label ---- */
      mu_Color fg = is_active ? TH_TAB_TEXT_ACTIVE : TH_TAB_TEXT;
      mu_Font font = ctx->style->font;
      int tw = ctx->text_width(font, mgr->screens[i].name, -1);
      int th = ctx->text_height(font);
      mu_push_clip_rect(ctx, r);
      mu_draw_text(ctx, font, mgr->screens[i].name, -1,
                   mu_vec2(r.x + (r.w - tw) / 2, r.y + (r.h - th) / 2), fg);
      mu_pop_clip_rect(ctx);
    }

    /* ---- Click handling (only when not renaming this tab) ---- */
    if (mgr->rename_idx != i) {

      /* Left click */
      if (ctx->mouse_pressed == MU_MOUSE_LEFT && ctx->focus == id) {

        /* Double-click detection */
        int frame = ctx->frame;
        if (s_last_click_tab == i &&
            (frame - s_last_click_frame) < DBLCLICK_FRAMES)
        {
          /* Double-click → rename */
          mgr->rename_idx = i;
          snprintf(mgr->rename_buf, SCREEN_NAME_LEN, "%s", mgr->screens[i].name);
          s_last_click_tab = -1;
        } else {
          /* Single click → switch + start potential drag */
          mgr->active_idx = i;
          mgr->drag_idx = i;
          mgr->drag_start_x = ctx->mouse_pos.x;
          mgr->dragging = 0;
          changed = 1;
          s_last_click_tab = i;
          s_last_click_frame = frame;
        }
      }

      /* Right-click → open close confirmation popup */
      if (ctx->mouse_pressed == MU_MOUSE_RIGHT && ctx->focus == id && n > 1) {
        mgr->close_confirm_idx = i;
        mu_open_popup(ctx, "!tab_close_confirm");
        s_last_click_tab = -1;
      }
    }

    /* ---- Active tab indicator ---- */
    if (is_active) {
      mu_draw_rect(ctx, mu_rect(r.x, r.y + r.h - 2, r.w, 2), TH_HEADER_TEXT);
    }

    /* Tab separator */
    mu_draw_rect(ctx, mu_rect(r.x + r.w - 1, r.y + 2, 1, r.h - 4), TH_SEPARATOR);
  }

  /* ---- Drag-and-drop logic ---- */
  if (mgr->drag_idx >= 0) {
    if (ctx->mouse_down & MU_MOUSE_LEFT) {
      int dx = ctx->mouse_pos.x - mgr->drag_start_x;
      if (dx < 0) dx = -dx;

      /* Activate drag after threshold */
      if (!mgr->dragging && dx > DRAG_THRESHOLD) {
        mgr->dragging = 1;
      }

      if (mgr->dragging) {
        /* Find which tab slot the cursor is over */
        mgr->drag_target = -1;
        for (int i = 0; i < n; i++) {
          int mid = tab_rects[i].x + tab_rects[i].w / 2;
          if (ctx->mouse_pos.x < mid) {
            mgr->drag_target = i;
            break;
          }
        }
        if (mgr->drag_target < 0) mgr->drag_target = n - 1;

        /* Draw drop indicator line */
        if (mgr->drag_target != mgr->drag_idx) {
          int dx_pos;
          if (mgr->drag_target <= 0) {
            dx_pos = tab_rects[0].x;
          } else if (mgr->drag_target >= n) {
            dx_pos = tab_rects[n - 1].x + tab_rects[n - 1].w;
          } else {
            dx_pos = tab_rects[mgr->drag_target].x;
          }
          mu_draw_rect(ctx,
            mu_rect(dx_pos - 1, tab_rects[0].y, 3, tab_h),
            TH_HEADER_TEXT);
        }
      }
    } else {
      /* Mouse released — perform swap if dragging */
      if (mgr->dragging && mgr->drag_target >= 0 &&
          mgr->drag_target != mgr->drag_idx)
      {
        /* Move drag_idx to drag_target via bubble swap */
        int src = mgr->drag_idx;
        int dst = mgr->drag_target;
        if (src < dst) {
          for (int i = src; i < dst; i++)
            screen_mgr_swap(mgr, i, i + 1);
        } else {
          for (int i = src; i > dst; i--)
            screen_mgr_swap(mgr, i, i - 1);
        }
        changed = 1;
      }
      mgr->drag_idx = -1;
      mgr->drag_target = -1;
      mgr->dragging = 0;
    }
  }

  /* ---- [+] Add Screen button ---- */
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
        int new_idx = screen_mgr_add(mgr, NULL);
        if (new_idx >= 0) {
          mgr->rename_idx = new_idx;
          snprintf(mgr->rename_buf, SCREEN_NAME_LEN, "%s",
                   mgr->screens[new_idx].name);
        }
        changed = 1;
      }
    }
  }

  /* spacer */
  mu_layout_next(ctx);

  /* ---- Close Confirmation Popup ---- */
  if (mgr->close_confirm_idx >= 0) {
    if (mu_begin_popup(ctx, "!tab_close_confirm")) {
      mu_layout_row(ctx, 1, (int[]){ 200 }, 0);
      char msg[96];
      snprintf(msg, sizeof(msg), "Close \"%s\"?",
               mgr->screens[mgr->close_confirm_idx].name);
      mu_label(ctx, msg);

      mu_layout_row(ctx, 2, (int[]){ 60, 60 }, 0);
      if (mu_button(ctx, "Yes")) {
        screen_mgr_remove(mgr, mgr->close_confirm_idx);
        mgr->close_confirm_idx = -1;
        changed = 1;
      }
      if (mu_button(ctx, "Cancel")) {
        mgr->close_confirm_idx = -1;
      }
      mu_end_popup(ctx);
    } else {
      /* Popup closed by clicking elsewhere — cancel */
      mgr->close_confirm_idx = -1;
    }
  }

  return changed;
}


/* ============================================================================
**  Filter Matching
** ============================================================================*/

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
      if (a >= 'A' && a <= 'Z') a = (char)(a + 32);
      if (b >= 'A' && b <= 'Z') b = (char)(b + 32);
      if (a != b) match = 0;
    }
    if (match) return 1;
  }
  return 0;
}


int screen_filter_check(const ScreenFilter *f, const Position *p) {
  switch (p->asset_class) {
    case ASSET_GOVT_BOND: if (!f->show_bonds) return 0; break;
    case ASSET_IRS:
    case ASSET_FRA:       if (!f->show_swaps) return 0; break;
    case ASSET_FUTURES:   if (!f->show_futures) return 0; break;
    case ASSET_SWAPTION:  if (!f->show_swaptions) return 0; break;
    default: break;
  }

  if (f->search[0]) {
    if (str_contains_ci(p->instrument, f->search)) return 1;
    if (str_contains_ci(p->cusip, f->search)) return 1;
    if (str_contains_ci(p->book, f->search)) return 1;
    if (str_contains_ci(p->desk, f->search)) return 1;
    return 0;
  }

  return 1;
}
