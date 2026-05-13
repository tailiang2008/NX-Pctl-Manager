// PtStateHeader — the 4-line play-timer state block shown at the top of both
// PlayTimerActivity and PlayTimerPerDayActivity. Reads pctl_play_timer_query
// and mirrors the text-console pt_print_state() lines.
// Copyright (C) 2026 Taylor.  GPLv3-or-later (see LICENSE).
#pragma once

#include <borealis.hpp>

class PtStateHeader : public brls::Box
{
  public:
    PtStateHeader();

    void refresh();

    static brls::View* create();

  private:
    BRLS_BIND(brls::Label, enabled_value,    "pt_enabled_value");
    BRLS_BIND(brls::Label, restricted_value, "pt_restricted_value");
    BRLS_BIND(brls::Label, remaining_value,  "pt_remaining_value");
    BRLS_BIND(brls::Label, configured_value, "pt_configured_value");
};
