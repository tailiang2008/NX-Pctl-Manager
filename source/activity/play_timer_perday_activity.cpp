// Copyright (C) 2026 Taylor.  GPLv3-or-later (see LICENSE).
#include "activity/play_timer_perday_activity.hpp"

#include <cstdio>
#include <cstring>
#include <fmt/format.h>
#include <string>

#include "action/pt_flow.hpp"
#include "util/numpad.hpp"

using namespace brls::literals;

namespace
{
constexpr const char* DAY_NAMES[7] = {
    "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
};

std::string fmt_minutes(u16 m)
{
    if (m == PT_DAY_NOLIMIT) return "no limit";
    if (m == 0)              return "no play";
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%u min", (unsigned)m);
    return buf;
}
}   // namespace

void PlayTimerPerDayActivity::onContentAvailable()
{
    // Hand each day cell to a numpad-stage lambda. Capturing the index directly
    // keeps things readable (vs. one shared lambda taking the cell ptr).
    auto wire_day = [this](brls::DetailCell* cell, int d) {
        cell->registerClickAction([this, d](brls::View*) {
            this->open_numpad_for(d);
            return true;
        });
    };
    wire_day(this->pt_d0, 0);
    wire_day(this->pt_d1, 1);
    wire_day(this->pt_d2, 2);
    wire_day(this->pt_d3, 3);
    wire_day(this->pt_d4, 4);
    wire_day(this->pt_d5, 5);
    wire_day(this->pt_d6, 6);

    this->pt_save->registerClickAction([this](brls::View*) {
        this->do_save();
        return true;
    });

    this->reload_from_service();
    this->state_header->refresh();
    this->rerender_day_labels();
}

void PlayTimerPerDayActivity::onResume()
{
    brls::Activity::onResume();
    // Coming back from a numpad applet round-trip — refresh visible state but
    // do NOT reload pending (it would wipe the user's staged edits).
    this->state_header->refresh();
    this->rerender_day_labels();
}

void PlayTimerPerDayActivity::reload_from_service()
{
    pctl_play_timer_query(&this->live);
    for (int i = 0; i < 7; i++) this->pending[i] = this->live.day_min[i];
}

void PlayTimerPerDayActivity::rerender_day_labels()
{
    brls::DetailCell* cells[7] = {
        this->pt_d0, this->pt_d1, this->pt_d2, this->pt_d3,
        this->pt_d4, this->pt_d5, this->pt_d6,
    };
    for (int d = 0; d < 7; d++) {
        std::string text = fmt_minutes(this->pending[d]);
        if (this->pending[d] != this->live.day_min[d])
            text += "  (*)";
        cells[d]->setDetailText(text);
    }
}

void PlayTimerPerDayActivity::open_numpad_for(int d)
{
    // Seed numpad: use current pending value, or 60 when "no limit" (the user
    // is presumably trying to put a value on this day, so no-limit isn't a
    // useful starting point).
    u16 seed = (this->pending[d] == PT_DAY_NOLIMIT) ? 60 : this->pending[d];
    auto v = numpad::prompt_minutes(
        fmt::format("{} limit  (0 = no play that day)", DAY_NAMES[d]),
        seed);
    if (!v.has_value()) return;   // cancelled
    this->pending[d] = *v;
    this->rerender_day_labels();
}

void PlayTimerPerDayActivity::do_save()
{
    bool changed = false;
    for (int i = 0; i < 7; i++)
        if (this->pending[i] != this->live.day_min[i]) { changed = true; break; }

    if (!changed) {
        brls::Application::notify("No changes to save.");
        return;
    }

    // Snapshot pending now so the gate's async callback uses a stable value
    // (the user could keep editing during the dialog — borealis blocks input
    // to this Activity while a Dialog is open, but capturing by value is
    // belt-and-braces anyway).
    u16 snapshot[7];
    std::memcpy(snapshot, this->pending, sizeof(snapshot));

    pt_flow::ready_to_write([this, snapshot](bool ok, bool did_unlock) {
        if (!ok) {
            // Declined / unlock failed; pending stays, refresh labels in case
            // `live` shifted while the gate was up (state_header re-renders too).
            this->reload_from_service();
            this->state_header->refresh();
            this->rerender_day_labels();
            return;
        }
        Result rc = pctl_play_timer_set_days(snapshot);

        // Re-read live and re-seed pending — clears all (*) markers on success.
        this->reload_from_service();
        this->state_header->refresh();
        this->rerender_day_labels();

        if (R_FAILED(rc)) {
            brls::Application::notify(fmt::format(
                "Failed to write the per-day limits (error 0x{:08X}).",
                (unsigned)rc));
            return;
        }
        // Build a summary line: "Su: 60 min  Mo: 30 min  …"
        std::string summary = "Per-day limits written: ";
        for (int d = 0; d < 7; d++) {
            summary += fmt::format("{}: {}", DAY_NAMES[d], fmt_minutes(snapshot[d]));
            if (d < 6) summary += "  ";
        }
        brls::Application::notify(summary);
        if (did_unlock) {
            auto* d = new brls::Dialog("nx_pctl/play_timer/did_unlock_notice"_i18n);
            d->addButton("hints/ok"_i18n, [] {});
            d->open();
        }
    });
}
