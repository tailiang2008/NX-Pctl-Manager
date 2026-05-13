// Copyright (C) 2026 Taylor.  GPLv3-or-later (see LICENSE).
#include "view/status_panel.hpp"

#include <cstdio>
#include <string>

#include "app.hpp"
#include "util/pctl_ops_c.hpp"

namespace
{
// English labels — kept inline (i18n key clutter not worth it for short, fixed
// status strings). Switch to "@i18n/..." lookups when adding a second language.
constexpr const char* UNAVAILABLE = "(unavailable)";

std::string fmt_pin(const PctlStatus& s)
{
    if (!s.pin_length_ok) return UNAVAILABLE;
    if (s.pin_length == 0) return "not set";
    char buf[32];
    std::snprintf(buf, sizeof(buf), "set (%u digits)", (unsigned)s.pin_length);
    return buf;
}

// Reads as "Parental control enabled : <value>" — the value carries the
// configured-limit detail when the timer is on.
std::string fmt_play_timer(const PtState& pt)
{
    if (!pt.valid)    return UNAVAILABLE;
    if (!pt.enabled)  return "no";

    bool any = false, uniform = true;
    for (int i = 0; i < 7; i++) {
        if (pt.day_min[i] != PT_DAY_NOLIMIT)  any = true;
        if (pt.day_min[i] != pt.day_min[0])   uniform = false;
    }

    if (!any)                                    return "yes";
    if (uniform && pt.day_min[0] == 0)           return "yes — every day blocked";
    if (uniform) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "yes — %u min/day", (unsigned)pt.day_min[0]);
        return buf;
    }
    return "yes — per-day limits";
}
}   // namespace

StatusPanel::StatusPanel()
{
    this->inflateFromXMLRes("xml/view/status_panel.xml");
}

void StatusPanel::refresh()
{
    if (!app::pctl_available()) {
        this->pin_value->setText(UNAVAILABLE);
        this->restrictions_value->setText(UNAVAILABLE);
        this->play_timer_value->setText(UNAVAILABLE);
        return;
    }

    PctlStatus s;
    pctl_status_fetch(&s);
    this->pin_value->setText(fmt_pin(s));
    this->restrictions_value->setText(
        s.restriction_enabled_ok
            ? (s.restriction_enabled ? "yes" : "no")
            : UNAVAILABLE);

    PtState pt;
    pctl_play_timer_query(&pt);
    this->play_timer_value->setText(fmt_play_timer(pt));
}

brls::View* StatusPanel::create() { return new StatusPanel(); }
