# bbg_tui — Bloomberg-style Terminal UI

A Bloomberg POMS-inspired position management screen built on a fork of
[rxi/microui](https://github.com/rxi/microui). Proof-of-concept for
eventual port to Odin.

## Quick Start

```bash
make run
```

### Prerequisites

| Platform | Install                         |
|----------|---------------------------------|
| macOS    | `brew install sdl2`             |
| Ubuntu   | `sudo apt install libsdl2-dev`  |
| Fedora   | `sudo dnf install SDL2-devel`   |

## Multi-Screen System

Each screen has **independent filters** — like having multiple Bloomberg
panels open on different products:

```
┌─ [ALL] [BONDS] [SWAPS] [VOL] [+] ──────────────────────────────────┐
│ Filter: [__________]  ☑Bond  ☑Swap  ☑Fut  ☑Vol                      │
│─────────────────────────────────────────────────────────────────────-│
│ INSTRUMENT     │CUSIP     │BOOK│DESK │NOTL(MM)│AVG PX│MKT PX│P&L(K)│
│─────────────────────────────────────────────────────────────────────-│
│ >> Rates Flow / US Rates                                             │
│ UST 2Y 4.25   │91282CKL8 │FLOW│USR  │  150.0 │99.875│99.920│ +67.5 │
│ UST 5Y 4.00   │91282CKM6 │FLOW│USR  │  -75.0 │98.500│98.125│ -28.1 │
│ ...                                                                  │
│═════════════════════════════════════════════════════════════════════ │
│ TOTAL (25)     │          │    │     │ 1234.5 │      │      │+XXX.X │
│ bbg_tui v0.1 | Positions: 25 | engine: microui 2.02                 │
└──────────────────────────────────────────────────────────────────────┘
```

### Controls

| Action                  | Input                              |
|-------------------------|------------------------------------|
| Switch screen           | Click tab or **F1-F4**             |
| Add screen              | Click **[+]**                      |
| Close screen            | Right-click tab (min 1 remains)    |
| Filter by asset class   | Toggle checkboxes per screen       |
| Search instruments      | Type in filter textbox             |
| Scroll positions        | Mouse wheel in grid                |
| Move window             | Drag title bar                     |
| Resize window           | Drag bottom-right corner           |

## Project Structure

```
bbg_tui/
├── Makefile
├── README.md
│
├── lib/                      # UI library (microui fork)
│   ├── bbg_tui.h             # Public header (wraps microui.h)
│   ├── microui.h             # Modified microui (bumped limits)
│   └── microui.c             # ← copy from rxi/microui (unmodified)
│
├── src/                      # Application modules
│   ├── theme.h               # Bloomberg color palette + style
│   ├── data.h                # Position data model
│   ├── data.c                # Sample data + market sim
│   ├── table.h               # Per-cell table rendering helpers
│   ├── table.c               # Bypasses mu_label for colored cells
│   ├── screen.h              # Multi-screen manager (tabs, filters)
│   ├── screen.c              # Tab bar rendering + filter logic
│   ├── poms.h                # POMS grid renderer interface
│   └── poms.c                # Column headers, rows, summary, status
│
└── demo/                     # SDL2/OpenGL backend
    ├── main.c                # Entry point, event loop, screen setup
    ├── renderer.h            # ← copy from rxi/microui demo/
    ├── renderer.c            # ← copy from rxi/microui demo/
    └── atlas.inl             # ← copy from rxi/microui demo/
```

## Modified microui Limits

| Define                  | Upstream | bbg_tui | Reason                          |
|-------------------------|----------|---------|--------------------------------|
| `MU_COMMANDLIST_SIZE`   | 256 KB   | 1 MB    | 25 rows × 15 cols = 375 cells  |
| `MU_CONTAINERPOOL_SIZE` | 48       | 128     | Multiple panels + tab screens   |
| `MU_TREENODEPOOL_SIZE`  | 48       | 128     | Book/desk grouping hierarchy    |
| `MU_MAX_WIDTHS`         | 16       | 32      | 15 columns, room to grow        |

## Architecture Notes

### Per-Cell Color Control

microui's `mu_label()` uses the global `style->colors[MU_COLOR_TEXT]` — useless
for a blotter where P&L cells are green/red, stale prices are yellow, etc.

The solution in `table.h` / `table.c`: bypass `mu_label()` entirely.
Each cell calls `mu_layout_next()` to consume a layout slot, then draws
directly with `mu_draw_rect()` (background) + `mu_draw_text()` (content):

```c
// table.c — per-cell colored rendering
void tbl_cell(mu_Context *ctx, const char *text,
              mu_Color bg, mu_Color fg, int opt)
{
    mu_Rect r = mu_layout_next(ctx);   // consume layout slot
    mu_draw_rect(ctx, r, bg);           // cell background
    // ... compute text position based on opt (left/right/center) ...
    mu_draw_text(ctx, font, text, -1, pos, fg);  // cell text
}
```

### Horizontal Scrolling

microui *does* have horizontal scrollbar code — the `scrollbar` macro uses
token pasting to handle both axes. It triggers when `content_size.x > body.w`.
With 15 fixed-width columns summing to ~960px, it'll activate when the window
is narrower than the total column width.

The remaining issue is **synchronized scroll**: column headers sit outside the
scrollable panel, so they don't move with horizontal scroll. The fix (TODO) is
to read the grid panel's `scroll.x` and offset the header drawing by it:

```c
mu_Container *grid = mu_get_container(ctx, "grid");
int hscroll = grid ? grid->scroll.x : 0;
// offset header cell positions by -hscroll
```

### Multi-Screen Architecture

Each `Screen` struct owns its own `ScreenFilter` with independent:
- Asset class toggles (bond/swap/fut/vol)
- Text search string
- Selected row state

The `ScreenManager` renders a tab bar and dispatches `poms_render()` with
the active screen's filter. Screens are created with presets:

```c
screen_mgr_add_preset(&mgr, "BONDS", 1, 0, 0, 0);  // bonds only
screen_mgr_add_preset(&mgr, "SWAPS", 0, 1, 0, 0);  // swaps only
```

## Roadmap → Odin Port

1. **C prototype** (current) — validate layout, colors, multi-screen
2. **bbg_tui lib divergence** — add hscroll sync, row selection, column sort
3. **Odin translation** — direct port of `microui.h/c` → `bbg_tui.odin`
   - `mu_stack` → `[N]T` fixed arrays
   - Function pointers → `proc` type
   - `mu_Context` → Odin struct with methods
   - Replace SDL2/GL renderer with `vendor:raylib` or `vendor:sdl2`
4. **Odin-native features** — SIMD aggregation, arena allocators, etc.

