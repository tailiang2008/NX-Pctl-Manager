// PlayTimerPerDayActivity — pushed from PlayTimerActivity. Each of 7 day cells
// stages an edit into `pending[]`; the 8th cell writes all 7 via cmd 195101.
// Pressing B pops without saving (default Activity behavior).
// Copyright (C) 2026 Taylor.  GPLv3-or-later (see LICENSE).
#pragma once

#include <borealis.hpp>

#include "view/pt_state_header.hpp"
#include "util/pctl_ops_c.hpp"   // for u16 / PtState

class PlayTimerPerDayActivity : public brls::Activity
{
  public:
    CONTENT_FROM_XML_RES("activity/play_timer_perday.xml");

    void onContentAvailable() override;
    void onResume() override;

  private:
    // State: `live` is the last-read config; `pending` is the staged config
    // (seeded from `live` on entry / after a successful Save).
    PtState live   = {};
    u16     pending[7] = { PT_DAY_NOLIMIT, PT_DAY_NOLIMIT, PT_DAY_NOLIMIT,
                           PT_DAY_NOLIMIT, PT_DAY_NOLIMIT, PT_DAY_NOLIMIT, PT_DAY_NOLIMIT };

    void reload_from_service();   // refresh `live` and reseed `pending`
    void rerender_day_labels();   // update each day cell's detailText (+ "(*)")
    void open_numpad_for(int d);  // pop swkbd, stage on confirm
    void do_save();               // gate → write → toast

    BRLS_BIND(PtStateHeader,    state_header, "pt_state_header");
    BRLS_BIND(brls::DetailCell, pt_d0,        "pt_d0");
    BRLS_BIND(brls::DetailCell, pt_d1,        "pt_d1");
    BRLS_BIND(brls::DetailCell, pt_d2,        "pt_d2");
    BRLS_BIND(brls::DetailCell, pt_d3,        "pt_d3");
    BRLS_BIND(brls::DetailCell, pt_d4,        "pt_d4");
    BRLS_BIND(brls::DetailCell, pt_d5,        "pt_d5");
    BRLS_BIND(brls::DetailCell, pt_d6,        "pt_d6");
    BRLS_BIND(brls::DetailCell, pt_save,      "pt_save");
};
