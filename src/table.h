/*
** table.h — Table cell rendering helpers for bbg_tui
**
** Bypasses mu_label() to give per-cell background and text color control.
** Each function calls mu_layout_next() internally, consuming one cell slot
** from the current mu_layout_row().
*/

#ifndef TABLE_H
#define TABLE_H

#include "bbg_tui.h"

/* ---- Draw a text cell with explicit foreground color ---- */
void tbl_cell_text(mu_Context *ctx, const char *text, mu_Color fg, int opt);

/* ---- Draw a text cell with background + foreground ---- */
void tbl_cell(mu_Context *ctx, const char *text, mu_Color bg, mu_Color fg, int opt);

/* ---- Draw a numeric cell, right-aligned, with auto P&L coloring ---- */
void tbl_cell_pnl(mu_Context *ctx, double val, const char *fmt,
                   mu_Color bg);

/* ---- Draw a numeric cell, right-aligned, specified color ---- */
void tbl_cell_num(mu_Context *ctx, double val, const char *fmt,
                  mu_Color bg, mu_Color fg);

/* ---- Draw an empty cell with background ---- */
void tbl_cell_empty(mu_Context *ctx, mu_Color bg);

/* ---- Draw a column separator line on the right edge of rect ---- */
void tbl_separator(mu_Context *ctx, mu_Rect r, mu_Color color);

#endif
