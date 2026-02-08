/*
** poms.h — POMS Position Grid Rendering
**
** Renders a single POMS screen (grid + filters + summary) for a given
** Screen and PositionBook. Caller (main.c) manages the screen manager
** and tab bar; this module just draws the content.
*/

#ifndef POMS_H
#define POMS_H

#include "bbg_tui.h"
#include "data.h"
#include "screen.h"

/* ---- Render the POMS grid for the active screen ---- */
void poms_render(mu_Context *ctx, Screen *scr, PositionBook *book, int tick);

#endif
