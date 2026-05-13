// PlayTimerActivity — the play-timer sub-menu (entered from the main menu's
// "Play timer (daily limit)" cell). State header on top, 3 action cells
// (set-all / per-day / remove) below.
// Copyright (C) 2026 Taylor.  GPLv3-or-later (see LICENSE).
#pragma once

#include <borealis.hpp>

#include "view/pt_state_header.hpp"

class PlayTimerActivity : public brls::Activity
{
  public:
    CONTENT_FROM_XML_RES("activity/play_timer.xml");

    void onContentAvailable() override;
    void onResume() override;

  private:
    BRLS_BIND(PtStateHeader,    state_header, "pt_state_header");
    BRLS_BIND(brls::DetailCell, pt_set_all,   "pt_set_all");
    BRLS_BIND(brls::DetailCell, pt_per_day,   "pt_per_day");
    BRLS_BIND(brls::DetailCell, pt_remove,    "pt_remove");
    BRLS_BIND(brls::DetailCell, pt_diag,      "pt_diag");   // PROBE-gated; see .cpp
};
