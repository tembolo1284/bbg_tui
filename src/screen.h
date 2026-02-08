/*
** screen.h — Multi-Screen Manager
**
** Manages multiple independent POMS views, each with its own filter state.
** Think Bloomberg's panel system: each screen can focus on different
** products/books while sharing the same underlying position book.
**
** Navigation:
**   - Tab bar at top (click to switch)
**   - [+] button to add a new screen
**   - [x] on each tab to close (minimum 1 screen)
**   - F1-F4 keyboard shortcuts for first 4 screens
*/

#ifndef SCREEN_H
#define SCREEN_H

#include "bbg_tui.h"
#include "data.h"

#define MAX_SCREENS    8
#define SCREEN_NAME_LEN 32

/* ---- Per-Screen Filter State ---- */
typedef struct {
  char  search[64];
  int   show_bonds;
  int   show_swaps;
  int   show_futures;
  int   show_swaptions;
} ScreenFilter;

/* ---- Single Screen ---- */
typedef struct {
  char          name[SCREEN_NAME_LEN];  /* tab label: "POMS 1", "Bonds", etc */
  ScreenFilter  filter;
  int           selected_row;           /* -1 = none */
  int           active;                 /* is this slot in use? */
} Screen;

/* ---- Screen Manager ---- */
typedef struct {
  Screen  screens[MAX_SCREENS];
  int     count;
  int     active_idx;         /* which screen is currently displayed */
  int     next_id;            /* auto-incrementing screen ID */

  /* ---- Rename state ---- */
  int     rename_idx;         /* which tab is being renamed (-1 = none) */
  char    rename_buf[SCREEN_NAME_LEN];

  /* ---- Drag-and-drop state (WIP) ---- */
  int     drag_idx;           /* tab being dragged (-1 = none) */
  int     drag_target;        /* insertion point (-1 = none) */
  int     drag_start_x;       /* mouse x at drag start */
  int     dragging;            /* 1 = drag in progress */
} ScreenManager;

/* ---- Initialize screen manager with one default screen ---- */
void screen_mgr_init(ScreenManager *mgr);

/* ---- Add a new screen with given name and filter preset ---- */
int  screen_mgr_add(ScreenManager *mgr, const char *name);

/* ---- Add a screen with a specific asset class filter preset ---- */
int  screen_mgr_add_preset(ScreenManager *mgr, const char *name,
                            int bonds, int swaps, int futures, int swaptions);

/* ---- Remove screen at index (won't remove last screen) ---- */
void screen_mgr_remove(ScreenManager *mgr, int idx);

/* ---- Get active screen ---- */
Screen* screen_mgr_active(ScreenManager *mgr);

/* ---- Render the tab bar. Returns 1 if active screen changed. ---- */
int  screen_mgr_tab_bar(ScreenManager *mgr, mu_Context *ctx);

/* ---- Check if a position passes the active screen's filter ---- */
int  screen_filter_check(const ScreenFilter *f, const Position *p);

/* ---- Swap two screens (for drag-and-drop reordering) ---- */
void screen_mgr_swap(ScreenManager *mgr, int a, int b);

#endif
