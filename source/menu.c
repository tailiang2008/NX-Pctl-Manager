// NX-Pctl-Manager — text-console menu UI (see menu.h).
// Copyright (C) 2026 Taylor.  This program is free software under the GNU
// General Public License v3 or later; it comes with NO WARRANTY. See the
// LICENSE file or <https://www.gnu.org/licenses/gpl-3.0.html> for details.
#include "menu.h"
#include <stdio.h>

// The console is double-buffered and consoleUpdate() syncs to vblank, so the
// standard pattern here is: handle input, redraw the whole screen, present —
// every frame. Screens that just wait for B redraw once and then call
// consoleUpdate() each frame purely to throttle the loop.

static PadState *s_pad;   // set by menu_run; valid for the lifetime of the menu (and its actions)

void menu_clear(void)
{
    printf("\x1b[2J\x1b[1;1H");
}

bool menu_confirm(const char *prompt)
{
    menu_clear();
    printf("=== Confirm ===\n\n");
    printf("%s\n\n", prompt ? prompt : "Are you sure?");
    printf("A: confirm    B: cancel\n");
    consoleUpdate(NULL);

    while (appletMainLoop()) {
        padUpdate(s_pad);
        u64 k = padGetButtonsDown(s_pad);
        if (k & HidNpadButton_A) return true;
        if (k & HidNpadButton_B) return false;
        consoleUpdate(NULL);
    }
    return false;   // appletMainLoop() ended (HOME / power) — treat as "no"
}

PadState *menu_current_pad(void)
{
    return s_pad;
}

// Pumps the applet loop until one of the buttons in `mask` is pressed; returns the
// pressed bit(s), or 0 if the app is closing (HOME / power). Used only by menu_wait_back.
static u64 menu_wait_for(u64 button_mask)
{
    while (appletMainLoop()) {
        padUpdate(s_pad);
        u64 k = padGetButtonsDown(s_pad) & button_mask;
        if (k) return k;
        consoleUpdate(NULL);
    }
    return 0;
}

void menu_wait_back(void)
{
    printf("\nPress B to go back.\n");
    consoleUpdate(NULL);
    menu_wait_for(HidNpadButton_B);
}

void menu_run(const char *title, menu_draw_fn draw_header,
              const MenuItem *items, size_t count, PadState *pad,
              const char *action_label, const char *back_label,
              size_t *cursor_inout)
{
    if (count == 0) return;
    s_pad = pad;
    padUpdate(pad);          // discard whatever was held when we entered (e.g. the launch press)

    size_t cursor = cursor_inout ? *cursor_inout : 0;
    if (cursor >= count) cursor = 0;             // count may have shrunk between re-entries
    const char *a = action_label ? action_label : "confirm";
    const char *b = back_label   ? back_label   : "exit";

    while (appletMainLoop()) {
        padUpdate(pad);
        u64 k = padGetButtonsDown(pad);

        if (k & HidNpadButton_B) break;
        if (k & HidNpadButton_AnyUp)   cursor = (cursor + count - 1) % count;
        if (k & HidNpadButton_AnyDown) cursor = (cursor + 1) % count;
        if ((k & HidNpadButton_A) && items[cursor].action) {
            bool go = true;
            if (items[cursor].confirm_prompt) go = menu_confirm(items[cursor].confirm_prompt);
            if (go) items[cursor].action();
        }

        menu_clear();
        if (title && *title) printf("%s\n\n", title);
        if (draw_header) draw_header();
        printf("\n");
        for (size_t i = 0; i < count; i++)
            printf("  %s %s\n", (i == cursor) ? ">" : " ", items[i].label);
        printf("\nUp/Down: move   A: %s   B: %s\n", a, b);
        consoleUpdate(NULL);
    }

    if (cursor_inout) *cursor_inout = cursor;
}
