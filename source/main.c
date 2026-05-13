// NX-Pctl-Manager — configure the Switch daily play-time limit offline, plus
// manage parental controls (set / change / delete the PIN, unlink the companion
// app, view status). Requires CFW (Atmosphere).
// Copyright (C) 2026 Taylor.  This program is free software under the GNU
// General Public License v3 or later; it comes with NO WARRANTY. See the
// LICENSE file or <https://www.gnu.org/licenses/gpl-3.0.html> for details.
#include <switch.h>
#include <stdio.h>
#include <stdlib.h>

#include "pctl_ops.h"
#include "menu.h"

#define APP_TITLE "=== NX-Pctl-Manager ==="

static PctlStatus s_status;   // refreshed on launch and after every action

static void refresh_status(void)
{
    pctl_status_fetch(&s_status);
}

static void draw_status_panel(void)
{
    printf("Status:\n");

    if (s_status.safety_level_ok)
        printf("  Safety level         : %s\n", pctl_safety_level_name(s_status.safety_level));
    else
        printf("  Safety level         : (unavailable)\n");

    if (s_status.pin_length_ok) {
        if (s_status.pin_length) printf("  PIN                  : set (%u digits)\n", s_status.pin_length);
        else                     printf("  PIN                  : not set\n");
    } else {
        printf("  PIN                  : (unavailable)\n");
    }

    if (s_status.restriction_enabled_ok)
        printf("  Restrictions enabled : %s\n", s_status.restriction_enabled ? "yes" : "no");
    else
        printf("  Restrictions enabled : (unavailable)\n");
}

static void act_refresh(void)
{
    refresh_status();
    menu_clear();
    printf("Status refreshed.\n");
    menu_wait_back();
}

static void act_set_pin(void)
{
    menu_clear();
    printf("Opening the system parental-controls screen...\n");
    consoleUpdate(NULL);

    Result rc = pctl_set_pin();

    menu_clear();
    if (R_SUCCEEDED(rc)) printf("Parental control PIN updated.\n");
    else                 printf("Could not update the PIN (error 0x%08X).\n", (unsigned int)rc);
    refresh_status();
    menu_wait_back();
}

static void act_delete_pc(void)
{
    Result rc = pctl_delete_parental_controls();

    menu_clear();
    if (R_SUCCEEDED(rc)) printf("Parental controls deleted.\n");
    else                 printf("Could not delete parental controls (error 0x%08X).\n", (unsigned int)rc);
    refresh_status();
    menu_wait_back();
}

static void act_delete_pairing(void)
{
    Result rc = pctl_delete_pairing();

    menu_clear();
    if (R_SUCCEEDED(rc)) printf("Companion app unlinked.\n");
    else                 printf("Could not unlink the companion app (error 0x%08X).\n", (unsigned int)rc);
    refresh_status();
    menu_wait_back();
}

// ----- Play timer: set the daily play-time limit (sub-menu of presets) -----

#ifdef PCTL_PROBE
static void save_report(const char *text)
{
    FILE *f = fopen("/nx_pctl_probe.txt", "w");
    if (f) { fputs(text, f); fclose(f); }
}
#endif

static PtState g_pt;            // cached play-timer state, refreshed on entry and after each action
static bool    s_pt_did_unlock; // set by pt_ready_to_write() when it had to turn parental controls off
                                // for the write; read by the writers to tweak their "done" message

static void pt_refresh(void) { pctl_play_timer_query(&g_pt); }

// Tacked onto a successful-write message when the write went through pt_ready_to_write()'s
// "turn parental controls off temporarily" path; no-op otherwise.
static void pt_did_unlock_notice(void)
{
    if (s_pt_did_unlock)
        printf("\nParental controls are temporarily off. Return to the main menu;\n"
               "the new limit takes effect once parental controls are active again.\n");
}

static const char *const PT_DAY[7] = {
    "Sunday   ", "Monday   ", "Tuesday  ", "Wednesday", "Thursday ", "Friday   ", "Saturday "
};

// Renders one per-day value: PT_DAY_NOLIMIT -> "no limit", 0 -> "no play", else "N min".
static const char *pt_minstr(u16 m, char *buf, size_t n)
{
    if      (m == PT_DAY_NOLIMIT) snprintf(buf, n, "no limit");
    else if (m == 0)              snprintf(buf, n, "no play");
    else                          snprintf(buf, n, "%u min", (unsigned)m);
    return buf;
}

static void pt_print_state(void)
{
    char tmp[16];
    printf("Play timer enabled : %s\n", g_pt.enabled ? "yes" : "no");
    printf("Limit reached today: %s\n", g_pt.restricted ? "yes ('time's up' screen may be active)" : "no");
    if (g_pt.remaining_ns)
        printf("Today's remaining  : ~%llu min  (raw 0x%016llX)\n",
               (unsigned long long)(g_pt.remaining_ns / 60000000000ULL), (unsigned long long)g_pt.remaining_ns);
    else
        printf("Today's remaining  : 0  (often reads 0 when no game is running)\n");

    if (!g_pt.valid) { printf("Configured limits  : (could not read)\n"); return; }

    bool uniform = true, anyset = false;
    for (int i = 0; i < 7; i++) {
        if (g_pt.day_min[i] != PT_DAY_NOLIMIT) anyset = true;
        if (g_pt.day_min[i] != g_pt.day_min[0]) uniform = false;
    }
    if (!anyset)      printf("Configured limit   : not set (timer off)\n");
    else if (uniform) printf("Configured limit   : %s (all days)\n", pt_minstr(g_pt.day_min[0], tmp, sizeof tmp));
    else {
        static const char *const ab[7] = { "Su", "Mo", "Tu", "We", "Th", "Fr", "Sa" };
        printf("Configured limits  :");
        for (int i = 0; i < 7; i++) {
            if      (g_pt.day_min[i] == PT_DAY_NOLIMIT) printf(" %s-", ab[i]);
            else if (g_pt.day_min[i] == 0)              printf(" %sX", ab[i]);
            else                                        printf(" %s%u", ab[i], g_pt.day_min[i]);
        }
        printf("\n  ( - = no limit,  X = blocked all day,  number = minutes )\n");
    }
}

static void pt_draw_header(void)
{
    pt_print_state();
    printf("\n0 min = that day blocked (not 'no limit'). Use 'Remove play-time\n");
    printf("limit' to turn the timer OFF. If it's active, setting a limit first\n");
    printf("briefly turns parental controls off (you confirm) to write it.\n\n");
}

// Pops the on-screen number pad. Returns true with *out set (clamped 0..1440) if
// the user entered a value; false if they cancelled or the keyboard couldn't open.
static bool pt_prompt_minutes(const char *header, u16 current, u16 *out)
{
    SwkbdConfig kbd;
    if (R_FAILED(swkbdCreate(&kbd, 0))) return false;
    swkbdConfigMakePresetDefault(&kbd);
    swkbdConfigSetType(&kbd, SwkbdType_NumPad);
    swkbdConfigSetStringLenMin(&kbd, 1);
    swkbdConfigSetStringLenMax(&kbd, 4);
    swkbdConfigSetHeaderText(&kbd, header);
    swkbdConfigSetGuideText(&kbd, "minutes per day  (0 = no play that day; max 1440)");
    char initial[8];
    snprintf(initial, sizeof(initial), "%u", (unsigned)current);
    swkbdConfigSetInitialText(&kbd, initial);

    char outstr[16] = {0};
    Result rc = swkbdShow(&kbd, outstr, sizeof(outstr));
    swkbdClose(&kbd);
    if (R_FAILED(rc)) return false;            // cancelled

    long v = strtol(outstr, NULL, 10);
    if (v < 0)    v = 0;
    if (v > 1440) v = 1440;
    *out = (u16)v;
    return true;
}

static void pt_apply(u16 minutes)   // sets `minutes` on every day (0 == every day blocked)
{
    Result rc = pctl_play_timer_set_uniform(minutes);
    menu_clear();
    if (R_FAILED(rc)) {
        printf("Failed to write the play-time limit (error 0x%08X).\n", (unsigned int)rc);
        pt_refresh();
        menu_wait_back();
        return;
    }
    if (minutes) printf("Play-time limit written: %u minute(s)/day.\n", minutes);
    else         printf("Play-time limit written: 0 minutes/day — no play allowed any day.\n"
                        "(Use 'Remove play-time limit' to turn the timer OFF instead.)\n");
    pt_did_unlock_notice();
    pt_refresh();
    menu_wait_back();
}

static void pt_off(void)   // turn the play timer off entirely
{
    Result rc = pctl_play_timer_clear();
    menu_clear();
    if (R_SUCCEEDED(rc)) printf("Play timer turned off (no limits, no blocked days).\n");
    else                 printf("Failed (0x%08X).\n", (unsigned int)rc);
    pt_refresh();
    menu_wait_back();
}

static u16 pt_uniform_value(void)   // current limit if all 7 days share one, else 60 (a default)
{
    if (!g_pt.valid) return 60;
    for (int i = 1; i < 7; i++) if (g_pt.day_min[i] != g_pt.day_min[0]) return 60;
    return (g_pt.day_min[0] == PT_DAY_NOLIMIT) ? 60 : g_pt.day_min[0];
}

// Rewriting the play-timer config while the timer is active can lock the console and
// destabilise Atmosphere — so if it's active, ask the user, then temporarily turn off
// parental controls (UnlockRestrictionTemporarily / cmd 1201, which on fw 22.1.0 makes
// IsPlayTimerEnabled read false) before letting the write proceed. Sets s_pt_did_unlock
// when it did so (the writers use it for their "done" message). Returns true when it's
// safe to write now (timer wasn't active, or we just turned parental controls off),
// false if the user declined or the call didn't take (caller writes nothing then).
static bool pt_ready_to_write(void)
{
    s_pt_did_unlock = false;
    pt_refresh();
    if (!g_pt.enabled) return true;        // timer not active — write straight away

    if (!menu_confirm("The play timer is currently active.\n"
                      "Writing a new limit needs parental controls to be\n"
                      "temporarily turned off first.\n\n"
                      "Turn parental controls off temporarily and write now?"))
        return false;                       // declined — nothing written

    menu_clear();
    printf("Temporarily turning off parental controls...\n");
    consoleUpdate(NULL);
    Result rc = pctl_unlock_restriction_temporarily();
    pt_refresh();                           // re-read state after the call

    if (R_FAILED(rc)) {
        menu_clear();
        printf("Could not turn off parental controls (error 0x%08X).\n", (unsigned int)rc);
        printf("Nothing written.\n");
        menu_wait_back();
        return false;
    }
    if (g_pt.enabled) {                     // returned OK but the play timer still shows enabled — don't risk it
        menu_clear();
        printf("Tried to turn off parental controls, but the play timer still\n"
               "shows enabled. Nothing written — try again in a moment.\n");
        menu_wait_back();
        return false;
    }
    s_pt_did_unlock = true;
    return true;                            // parental controls off now — safe to write
}

static void pt_set_all(void)
{
    u16 v;
    if (!pt_prompt_minutes("Daily play-time limit for ALL days  (0 = block every day)", pt_uniform_value(), &v))
        return;   // cancelled
    if (!pt_ready_to_write()) return;   // play timer active -> asked the user to unlock; nothing written
    pt_apply(v);
}

#ifdef PCTL_PROBE
static void pt_diag(void)
{
    static char rep[6144];
    pctl_play_timer_dump(rep, sizeof(rep));
    save_report(rep);
    menu_clear();
    printf("%s\n(saved to sd:/nx_pctl_probe.txt)\n", rep);
    menu_wait_back();
}
#endif

// ---- per-day sub-menu: A on a day STAGES that day's limit; "Save" writes all 7 ----
// Edits go into g_pt_pending (seeded from the live config on entry); nothing touches the
// service until the user picks "Save per-day limits". B leaves without saving.
static char g_pt_day_labels[7][48];
static u16  g_pt_pending[7];

static void pt_rebuild_day_labels(void)
{
    char tmp[16];
    for (int d = 0; d < 7; d++) {
        bool changed = g_pt_pending[d] != g_pt.day_min[d];
        snprintf(g_pt_day_labels[d], sizeof(g_pt_day_labels[d]), "%s : %s%s",
                 PT_DAY[d], pt_minstr(g_pt_pending[d], tmp, sizeof tmp), changed ? "  (*)" : "");
    }
}

static void pt_set_day(int d)   // stage a new limit for day d (not written until "Save")
{
    char hdr[64];
    snprintf(hdr, sizeof(hdr), "%.9s limit  (0 = no play that day)", PT_DAY[d]);
    u16 pre = (g_pt_pending[d] == PT_DAY_NOLIMIT) ? 60 : g_pt_pending[d];
    u16 v;
    if (!pt_prompt_minutes(hdr, pre, &v)) return;   // cancelled
    g_pt_pending[d] = v;
    pt_rebuild_day_labels();
}

static void pt_d0(void) { pt_set_day(0); }
static void pt_d1(void) { pt_set_day(1); }
static void pt_d2(void) { pt_set_day(2); }
static void pt_d3(void) { pt_set_day(3); }
static void pt_d4(void) { pt_set_day(4); }
static void pt_d5(void) { pt_set_day(5); }
static void pt_d6(void) { pt_set_day(6); }

static void pt_save_perday(void)   // write the staged 7-day config (via pt_ready_to_write if the timer's active)
{
    bool changed = false;
    for (int i = 0; i < 7; i++) if (g_pt_pending[i] != g_pt.day_min[i]) changed = true;
    if (!changed) {
        menu_clear();
        printf("No changes to save.\n");
        menu_wait_back();
        return;
    }
    if (!pt_ready_to_write()) {   // play timer active -> asked to turn parental controls off; declined/failed -> keep staged, nothing written
        pt_rebuild_day_labels();  // pt_ready_to_write refreshed g_pt; keep the (*) markers in sync
        return;
    }

    Result rc = pctl_play_timer_set_days(g_pt_pending);
    pt_refresh();
    for (int i = 0; i < 7; i++) g_pt_pending[i] = g_pt.day_min[i];   // resync staged with what's now live (clears (*))
    pt_rebuild_day_labels();
    menu_clear();
    if (R_FAILED(rc)) {
        printf("Failed to write the per-day limits (error 0x%08X).\n", (unsigned int)rc);
        menu_wait_back();
        return;
    }
    char tmp[16];
    printf("Per-day play-time limits written:\n");
    for (int d = 0; d < 7; d++) printf("  %s : %s\n", PT_DAY[d], pt_minstr(g_pt.day_min[d], tmp, sizeof tmp));
    pt_did_unlock_notice();
    menu_wait_back();
}

static const MenuItem g_pt_perday_items[8] = {
    { g_pt_day_labels[0], NULL, pt_d0 },
    { g_pt_day_labels[1], NULL, pt_d1 },
    { g_pt_day_labels[2], NULL, pt_d2 },
    { g_pt_day_labels[3], NULL, pt_d3 },
    { g_pt_day_labels[4], NULL, pt_d4 },
    { g_pt_day_labels[5], NULL, pt_d5 },
    { g_pt_day_labels[6], NULL, pt_d6 },
    { "Save per-day limits", NULL, pt_save_perday },
};

static void pt_draw_perday_header(void)
{
    pt_print_state();
    printf("\nPress A to type a day's limit (0 = no play). Edits stage with '(*)';\n");
    printf("pick 'Save per-day limits' to write all 7, or B to discard. If the\n");
    printf("timer's active you'll first be asked to disable parental controls.\n\n");
}

static void pt_per_day(void)
{
    pt_refresh();
    for (int i = 0; i < 7; i++) g_pt_pending[i] = g_pt.day_min[i];   // seed staged config from the live one
    pt_rebuild_day_labels();
    menu_run("=== Per-day play-time limits ===", pt_draw_perday_header,
             g_pt_perday_items, 8, menu_current_pad());
}

static const MenuItem g_pt_items[] = {
    { "Set daily limit (all days)...",              NULL, pt_set_all },
    { "Per-day limits...",                          NULL, pt_per_day },
    { "Remove play-time limit", "Remove the play-time limit (all days)?", pt_off },
#ifdef PCTL_PROBE
    { "Dump current config -> sd:/nx_pctl_probe.txt", NULL, pt_diag },
#endif
};

static void act_play_timer(void)
{
    pt_refresh();
    menu_run("=== Play timer ===", pt_draw_header,
             g_pt_items, sizeof(g_pt_items) / sizeof(g_pt_items[0]), menu_current_pad());
    pctl_ops_reinit();   // a diagnostic dump may have closed the session; restore it
}

static const MenuItem g_items[] = {
    { "Refresh status",                    NULL, act_refresh },
    { "Play timer (daily limit)",          NULL, act_play_timer },
    { "Set / change parental control PIN", NULL, act_set_pin },
    { "Delete all parental controls",
      "Delete ALL parental controls?\n"
      "This removes the PIN and every restriction, and cannot be undone.",
      act_delete_pc },
    { "Unlink companion app",
      "Unlink the companion app (Nintendo Switch Parental Controls)\n"
      "from this console?",
      act_delete_pairing },
};

int main(int argc, char *argv[])
{
    (void)argc; (void)argv;

    consoleInit(NULL);

    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    PadState pad;
    padInitializeDefault(&pad);

    Result rc = pctl_ops_init();
    if (R_FAILED(rc)) {
        printf("\x1b[2J\x1b[1;1H");
        printf(APP_TITLE "\n\n");
        printf("Could not open the parental-control service (error 0x%08X).\n\n", (unsigned int)rc);
        printf("This homebrew only works on a console running CFW (Atmosphere).\n\n");
        printf("Press B to exit.\n");
        consoleUpdate(NULL);

        padUpdate(&pad);
        while (appletMainLoop()) {
            padUpdate(&pad);
            if (padGetButtonsDown(&pad) & HidNpadButton_B) break;
            consoleUpdate(NULL);
        }
        consoleExit(NULL);
        return 0;
    }

    refresh_status();
    menu_run(APP_TITLE, draw_status_panel, g_items, sizeof(g_items) / sizeof(g_items[0]), &pad);

    pctl_ops_exit();
    consoleExit(NULL);
    return 0;
}
