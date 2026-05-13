// Copyright (C) 2026 Taylor.  GPLv3-or-later (see LICENSE).
#include "activity/play_timer_activity.hpp"

#include <fmt/format.h>

#include "action/pt_flow.hpp"
#include "activity/play_timer_perday_activity.hpp"
#include "util/numpad.hpp"
#include "util/pctl_ops_c.hpp"

using namespace brls::literals;

namespace
{
// If the seven days share one value, return that minute count; otherwise return
// 60 as a sensible default for the numpad to land on. PT_DAY_NOLIMIT counts as
// "no useful starting value" → fall back to 60.
uint16_t guess_uniform_seed()
{
    PtState pt;
    pctl_play_timer_query(&pt);
    if (!pt.valid) return 60;
    for (int i = 1; i < 7; i++)
        if (pt.day_min[i] != pt.day_min[0]) return 60;
    return (pt.day_min[0] == PT_DAY_NOLIMIT) ? 60 : pt.day_min[0];
}

// Pop the "Parental controls are temporarily off — return to the main menu;
// the new limit takes effect once parental controls are active again." dialog.
// Called only after a write that went through the gate's unlock branch.
void show_did_unlock_notice()
{
    auto* d = new brls::Dialog("nx_pctl/play_timer/did_unlock_notice"_i18n);
    d->addButton("hints/ok"_i18n, [] {});
    d->open();
}
}   // namespace

void PlayTimerActivity::onContentAvailable()
{
    // Set daily limit (all days) — numpad → write-gate → cmd 195101.
    this->pt_set_all->registerClickAction([this](brls::View*) {
        auto v = numpad::prompt_minutes(
            "Daily play-time limit for ALL days  (0 = block every day)",
            guess_uniform_seed());
        if (!v.has_value()) return true;   // user cancelled
        uint16_t value = *v;

        pt_flow::ready_to_write([this, value](bool ok, bool did_unlock) {
            if (!ok) return;   // declined / failed — gate already toasted
            Result rc = pctl_play_timer_set_uniform(value);
            this->state_header->refresh();
            if (R_FAILED(rc)) {
                brls::Application::notify(fmt::format(
                    "Failed to write the play-time limit (error 0x{:08X}).",
                    (unsigned)rc));
                return;
            }
            if (value)
                brls::Application::notify(fmt::format(
                    "Play-time limit written: {} minute(s)/day.", value));
            else
                brls::Application::notify(
                    "Play-time limit written: 0 minutes/day — "
                    "no play allowed any day. "
                    "(Use 'Remove play-time limit' to turn the timer OFF instead.)");
            if (did_unlock) show_did_unlock_notice();
        });
        return true;
    });

    // Per-day limits — push the staging sub-Activity.
    this->pt_per_day->registerClickAction([](brls::View*) {
        brls::Application::pushActivity(new PlayTimerPerDayActivity());
        return true;
    });

    // Remove play-time limit — destructive, **deliberately skips** pt_flow's
    // write-gate (it IS the "turn the timer off" path; routing through 1201
    // would be circular). See CLAUDE.md / PROGRESS.md for the known crash
    // when called while the "time's up" lock screen is showing.
    this->pt_remove->registerClickAction([this](brls::View*) {
        auto* dialog = new brls::Dialog("nx_pctl/play_timer/dialog/remove/body"_i18n);
        dialog->addButton("hints/cancel"_i18n, [] {});
        dialog->addButton("nx_pctl/play_timer/dialog/remove/confirm"_i18n, [this]() {
            Result rc = pctl_play_timer_clear();
            this->state_header->refresh();
            if (R_SUCCEEDED(rc))
                brls::Application::notify("Play timer turned off.");
            else
                brls::Application::notify(fmt::format(
                    "Could not turn off the play timer (error 0x{:08X}).",
                    (unsigned)rc));
        });
        dialog->open();
        return true;
    });

    this->state_header->refresh();
}

void PlayTimerActivity::onResume()
{
    brls::Activity::onResume();
    this->state_header->refresh();
}
