// Copyright (C) 2026 Taylor.  GPLv3-or-later (see LICENSE).
#include "activity/main_activity.hpp"

#include "app.hpp"

using namespace brls::literals;   // for "key"_i18n

void MainActivity::onContentAvailable()
{
    // Refresh status — step (c): re-read pctl state and update the panel.
    // The status panel already refreshes on onResume; this lets the user force
    // a re-read without leaving the screen.
    this->item_refresh->registerClickAction([this](brls::View*) {
        this->status_panel->refresh();
        brls::Application::notify("nx_pctl/toast/status_refreshed"_i18n);
        return true;
    });

    // The rest still toast a placeholder until they're wired in (d)-(f).
    auto placeholder = [](brls::View*) {
        brls::Application::notify("nx_pctl/toast/todo"_i18n);
        return true;
    };
    this->item_play_timer->registerClickAction(placeholder);
    this->item_set_pin   ->registerClickAction(placeholder);
    this->item_delete_pc ->registerClickAction(placeholder);
    this->item_unlink    ->registerClickAction(placeholder);

    // First read of the status panel — onResume runs it again on every entry,
    // but a freshly created Activity doesn't get an onResume call.
    this->status_panel->refresh();
}

void MainActivity::onResume()
{
    brls::Activity::onResume();
    this->status_panel->refresh();
}
