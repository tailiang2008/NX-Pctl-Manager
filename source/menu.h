// NX-Pctl-Manager — a tiny cursor-driven menu on top of libnx's text console.
// Copyright (C) 2026 Taylor.  This program is free software under the GNU
// General Public License v3 or later; it comes with NO WARRANTY. See the
// LICENSE file or <https://www.gnu.org/licenses/gpl-3.0.html> for details.
#pragma once
#include <switch.h>
#include <stddef.h>

typedef void (*menu_action_fn)(void);   // run when the item is chosen
typedef void (*menu_draw_fn)(void);     // draws a header/status block at the top of every redraw

typedef struct {
    const char     *label;
    const char     *confirm_prompt;     // if non-NULL, a yes/no screen is shown before `action` runs
    menu_action_fn  action;
} MenuItem;

// Runs the menu loop until the user presses B. `pad` must already be configured
// (padConfigureInput) and initialized (padInitializeDefault). `title` and
// `draw_header` may be NULL.
//
// `action_label` / `back_label`: the verbs in the bottom hint
//   "A: <action>   B: <back>". Pass NULL for the defaults ("confirm" / "exit");
//   sub-menus typically pass back_label="back".
// `cursor_inout`: if non-NULL, the cursor starts at *cursor_inout and the
//   final position is written back when the user leaves — keep a static at each
//   call site to remember the spot across re-entries.
void menu_run(const char *title, menu_draw_fn draw_header,
              const MenuItem *items, size_t count, PadState *pad,
              const char *action_label, const char *back_label,
              size_t *cursor_inout);

// These may be called from a MenuItem action — a menu must be running.
bool menu_confirm(const char *prompt);   // A = confirm, B = cancel
void menu_wait_back(void);               // prints "Press B to go back." and waits for B
void menu_clear(void);                   // clears the screen and homes the cursor
PadState *menu_current_pad(void);        // the PadState menu_run is using — valid while a menu is running (e.g. to open a sub-menu from an action)
