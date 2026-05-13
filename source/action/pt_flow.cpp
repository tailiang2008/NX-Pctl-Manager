// Copyright (C) 2026 Taylor.  GPLv3-or-later (see LICENSE).
#include "action/pt_flow.hpp"

#include <borealis.hpp>
#include <fmt/format.h>

#include "util/pctl_ops_c.hpp"

using namespace brls::literals;

namespace pt_flow
{

void ready_to_write(std::function<void(bool, bool)> on_ready)
{
    PtState pt;
    pctl_play_timer_query(&pt);

    // Timer isn't running → write straight away.
    if (!pt.enabled) {
        on_ready(true, false);
        return;
    }

    auto* dialog = new brls::Dialog("nx_pctl/play_timer/gate/body"_i18n);
    dialog->addButton("hints/cancel"_i18n, [on_ready]() {
        on_ready(false, false);   // declined — caller writes nothing
    });
    dialog->addButton("nx_pctl/play_timer/gate/confirm"_i18n, [on_ready]() {
        Result rc = pctl_unlock_restriction_temporarily();
        // Re-read state: 1201 should have flipped IsPlayTimerEnabled to false.
        PtState after;
        pctl_play_timer_query(&after);

        if (R_FAILED(rc) || after.enabled) {
            // Either the cmd 1201 itself failed, or it returned OK but the
            // timer is still showing enabled (theoretically impossible — bail
            // anyway, don't risk writing while it's active).
            brls::Application::notify(fmt::format(
                "Could not turn off parental controls (error 0x{:08X}).",
                (unsigned)rc));
            on_ready(false, false);
            return;
        }
        on_ready(true, true);
    });
    dialog->open();
}

}   // namespace pt_flow
