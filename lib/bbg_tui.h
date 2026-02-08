/*
** bbg_tui.h — Bloomberg TUI Library
**
** Forked from rxi/microui v2.02. Currently a thin wrapper that re-exports
** microui's API. As the fork diverges (horizontal scroll, table widgets,
** keyboard nav), this becomes the canonical header.
**
** For now, microui.c compiles unchanged against this header via microui.h.
** The mu_ prefix is retained until the Odin port, at which point we'll
** rename to bbg_ namespace.
**
** Modified limits vs upstream microui:
**   MU_COMMANDLIST_SIZE   256KB -> 1MB    (dense grids)
**   MU_CONTAINERPOOL_SIZE 48    -> 128    (many panels)
**   MU_TREENODEPOOL_SIZE  48    -> 128    (deep hierarchy)
**   MU_MAX_WIDTHS         16    -> 32     (wide tables)
*/

#ifndef BBG_TUI_H
#define BBG_TUI_H

/* Pull in the actual implementation header (which has our modified limits) */
#include "microui.h"

/*
** Future additions for the bbg_tui fork:
**
** - bbg_table_begin() / bbg_table_end()     — virtual-scrolled grid widget
** - bbg_hscroll_panel()                      — horizontal scroll panel
** - bbg_kbd_nav()                            — arrow key cell navigation
** - bbg_column_sort()                        — click-to-sort headers
** - bbg_cell_edit()                          — inline cell editing
** - bbg_context_menu()                       — right-click menus
*/

#endif
