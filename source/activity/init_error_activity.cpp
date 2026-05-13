// Copyright (C) 2026 Taylor.  GPLv3-or-later (see LICENSE).
#include "activity/init_error_activity.hpp"

#include <fmt/format.h>

#include "app.hpp"

using namespace brls::literals;

void InitErrorActivity::onContentAvailable()
{
    // Show the raw pctlInitialize Result code so the user can copy it into a
    // bug report; the prose above + the footer "Exit" hint are enough context.
    this->error_code->setText(
        fmt::format("error 0x{:08X}", app::pctl_init_result()));

    // Override B → quit the app (we're the only Activity on the stack, so
    // popping would also quit, but explicitly registering changes the footer
    // hint from "Back" to "Exit" — the user's only meaningful action here).
    this->getContentView()->registerAction(
        "hints/exit"_i18n, brls::BUTTON_B,
        [](brls::View*) {
            brls::Application::quit();
            return true;
        });
}
