// Copyright (C) 2026 Taylor.  GPLv3-or-later (see LICENSE).
#include "view/pt_state_header.hpp"

#include <cstdio>
#include <string>

#include "util/pctl_ops_c.hpp"

namespace
{
std::string fmt_remaining(const PtState& pt)
{
    if (!pt.enabled)        return "unknown";   // no active timer to report against
    if (pt.remaining_ns) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "~%llu min",
                      (unsigned long long)(pt.remaining_ns / 60000000000ULL));
        return buf;
    }
    return "0";
}

std::string fmt_configured(const PtState& pt)
{
    if (!pt.valid) return "(unavailable)";

    bool any = false, uniform = true;
    for (int i = 0; i < 7; i++) {
        if (pt.day_min[i] != PT_DAY_NOLIMIT)  any = true;
        if (pt.day_min[i] != pt.day_min[0])   uniform = false;
    }
    if (!any)                            return "not set (timer off)";
    if (uniform && pt.day_min[0] == 0)   return "every day blocked";
    if (uniform) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%u min (all days)", (unsigned)pt.day_min[0]);
        return buf;
    }
    // Per-day breakdown: "Su 60  Mo 30  Tu 60  …" with - for no-limit, X for blocked.
    static const char* const ab[7] = { "Su", "Mo", "Tu", "We", "Th", "Fr", "Sa" };
    std::string out = "per-day:";
    for (int i = 0; i < 7; i++) {
        char part[12];
        if      (pt.day_min[i] == PT_DAY_NOLIMIT) std::snprintf(part, sizeof(part), " %s-", ab[i]);
        else if (pt.day_min[i] == 0)              std::snprintf(part, sizeof(part), " %sX", ab[i]);
        else                                      std::snprintf(part, sizeof(part), " %s%u", ab[i], (unsigned)pt.day_min[i]);
        out += part;
    }
    return out;
}
}   // namespace

PtStateHeader::PtStateHeader()
{
    this->inflateFromXMLRes("xml/view/pt_state_header.xml");
}

void PtStateHeader::refresh()
{
    PtState pt;
    pctl_play_timer_query(&pt);

    this->enabled_value->setText(pt.enabled ? "yes" : "no");
    this->restricted_value->setText(
        pt.restricted ? "yes ('time's up' screen may be active)" : "no");
    this->remaining_value->setText(fmt_remaining(pt));
    this->configured_value->setText(fmt_configured(pt));
}

brls::View* PtStateHeader::create() { return new PtStateHeader(); }
