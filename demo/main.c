/*
** main.c — bbg_tui POMS Demo Entry Point
**
** Multi-screen Bloomberg-style position manager.
**
** Controls:
**   Tab bar:     Click tabs to switch screens
**                Right-click a tab to close it
**                [+] to add a new screen
**   F1-F4:       Quick switch to screens 1-4
**   Grid:        Scroll to navigate positions
**   Filters:     Each screen has independent filters
**   Window:      Drag title bar, resize from bottom-right corner
*/

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "renderer.h"
#include "bbg_tui.h"
#include "theme.h"
#include "data.h"
#include "screen.h"
#include "poms.h"

/* ---- Global State ---- */
static PositionBook g_book;
static ScreenManager g_screens;
static int g_tick = 0;

/* ---- Keyboard / Mouse Maps ---- */

static const char button_map[256] = {
  [SDL_BUTTON_LEFT   & 0xff] = MU_MOUSE_LEFT,
  [SDL_BUTTON_RIGHT  & 0xff] = MU_MOUSE_RIGHT,
  [SDL_BUTTON_MIDDLE & 0xff] = MU_MOUSE_MIDDLE,
};

static const char key_map[256] = {
  [SDLK_LSHIFT    & 0xff] = MU_KEY_SHIFT,
  [SDLK_RSHIFT    & 0xff] = MU_KEY_SHIFT,
  [SDLK_LCTRL     & 0xff] = MU_KEY_CTRL,
  [SDLK_RCTRL     & 0xff] = MU_KEY_CTRL,
  [SDLK_LALT      & 0xff] = MU_KEY_ALT,
  [SDLK_RALT      & 0xff] = MU_KEY_ALT,
  [SDLK_RETURN    & 0xff] = MU_KEY_RETURN,
  [SDLK_BACKSPACE & 0xff] = MU_KEY_BACKSPACE,
};

/* ---- Text Measurement Callbacks ---- */

static int text_width_cb(mu_Font font, const char *text, int len) {
  (void)font;
  if (len == -1) len = strlen(text);
  return r_get_text_width(text, len);
}

static int text_height_cb(mu_Font font) {
  (void)font;
  return r_get_text_height();
}

/* ---- Process Frame ---- */

static void process_frame(mu_Context *ctx) {
  mu_begin(ctx);

  int win_flags = MU_OPT_NOCLOSE;
  if (mu_begin_window_ex(ctx, "POMS - Position Management",
                         mu_rect(5, 5, 960, 580), win_flags))
  {
    mu_Container *win = mu_get_current_container(ctx);
    win->rect.w = mu_max(win->rect.w, 800);
    win->rect.h = mu_max(win->rect.h, 300);

    /* ---- Tab Bar ---- */
    screen_mgr_tab_bar(&g_screens, ctx);

    /* ---- Separator below tabs ---- */
    mu_layout_row(ctx, 1, (int[]){ -1 }, 1);
    mu_draw_rect(ctx, mu_layout_next(ctx), TH_SEPARATOR);

    /* ---- Render Active Screen's POMS Grid ---- */
    Screen *active = screen_mgr_active(&g_screens);
    poms_render(ctx, active, &g_book, g_tick);

    mu_end_window(ctx);
  }

  mu_end(ctx);
}

/* ---- Handle F-key Screen Switching ---- */

static void handle_fkeys(SDL_Event *e, ScreenManager *mgr) {
  if (e->type != SDL_KEYDOWN) return;
  int idx = -1;
  switch (e->key.keysym.sym) {
    case SDLK_F1: idx = 0; break;
    case SDLK_F2: idx = 1; break;
    case SDLK_F3: idx = 2; break;
    case SDLK_F4: idx = 3; break;
  }
  if (idx >= 0 && idx < mgr->count) {
    mgr->active_idx = idx;
  }
}

/* ---- Main ---- */

int main(int argc, char **argv) {
  (void)argc; (void)argv;
  srand((unsigned)time(NULL));

  /* init data */
  data_init(&g_book);

  /* init screen manager with preset screens */
  screen_mgr_init(&g_screens);
  screen_mgr_add_preset(&g_screens, "BONDS",  1, 0, 0, 0);
  screen_mgr_add_preset(&g_screens, "SWAPS",  0, 1, 0, 0);
  screen_mgr_add_preset(&g_screens, "VOL",    0, 0, 0, 1);

  /* init SDL + renderer */
  SDL_Init(SDL_INIT_EVERYTHING);
  r_init();

  /* init microui */
  mu_Context *ctx = malloc(sizeof(mu_Context));
  mu_init(ctx);
  ctx->text_width  = text_width_cb;
  ctx->text_height = text_height_cb;
  theme_apply(ctx);

  /* main loop */
  for (;;) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      switch (e.type) {
        case SDL_QUIT:
          free(ctx);
          SDL_Quit();
          return 0;

        case SDL_MOUSEMOTION:
          mu_input_mousemove(ctx, e.motion.x, e.motion.y);
          break;

        case SDL_MOUSEWHEEL:
          mu_input_scroll(ctx, 0, e.wheel.y * -30);
          break;

        case SDL_TEXTINPUT:
          mu_input_text(ctx, e.text.text);
          break;

        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP: {
          int b = button_map[e.button.button & 0xff];
          if (b && e.type == SDL_MOUSEBUTTONDOWN)
            mu_input_mousedown(ctx, e.button.x, e.button.y, b);
          if (b && e.type == SDL_MOUSEBUTTONUP)
            mu_input_mouseup(ctx, e.button.x, e.button.y, b);
          break;
        }

        case SDL_KEYDOWN:
        case SDL_KEYUP: {
          /* F-key screen switching */
          handle_fkeys(&e, &g_screens);
          /* microui key handling */
          int c = key_map[e.key.keysym.sym & 0xff];
          if (c && e.type == SDL_KEYDOWN) mu_input_keydown(ctx, c);
          if (c && e.type == SDL_KEYUP)   mu_input_keyup(ctx, c);
          break;
        }
      }
    }

    /* tick simulation */
    g_tick++;
    data_simulate_tick(&g_book, g_tick);

    /* process UI */
    process_frame(ctx);

    /* render */
    r_clear(TH_BG);
    mu_Command *cmd = NULL;
    while (mu_next_command(ctx, &cmd)) {
      switch (cmd->type) {
        case MU_COMMAND_TEXT: r_draw_text(cmd->text.str, cmd->text.pos, cmd->text.color); break;
        case MU_COMMAND_RECT: r_draw_rect(cmd->rect.rect, cmd->rect.color); break;
        case MU_COMMAND_ICON: r_draw_icon(cmd->icon.id, cmd->icon.rect, cmd->icon.color); break;
        case MU_COMMAND_CLIP: r_set_clip_rect(cmd->clip.rect); break;
      }
    }
    r_present();

    SDL_Delay(16);
  }
}
