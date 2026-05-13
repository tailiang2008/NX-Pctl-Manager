// StatusPanel — the 4-row status block shown above the main menu (replaces the
// old text-console draw_status_panel). Reads pctl_status_fetch + the play-timer
// query and writes the four "*_value" labels.
// Copyright (C) 2026 Taylor.  GPLv3-or-later (see LICENSE).
#pragma once

#include <borealis.hpp>

class StatusPanel : public brls::Box
{
  public:
    StatusPanel();

    // Re-read the pctl state and update the labels. Cheap; safe to call on
    // every onResume of the hosting Activity.
    void refresh();

    static brls::View* create();

  private:
    BRLS_BIND(brls::Label, pin_value,          "pin_value");
    BRLS_BIND(brls::Label, restrictions_value, "restrictions_value");
    BRLS_BIND(brls::Label, play_timer_value,   "play_timer_value");
};
