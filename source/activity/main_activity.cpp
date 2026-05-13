// Copyright (C) 2026 Taylor.  GPLv3-or-later (see LICENSE).
#include "activity/main_activity.hpp"

#include "app.hpp"

using namespace brls::literals;   // for "key"_i18n

void MainActivity::onContentAvailable()
{
    // Step (b): each action item just toasts a placeholder. Real wiring lands
    // step by step in (c)-(f). Lambdas return true to mark the click handled.
    auto placeholder = [](brls::View*) {
        brls::Application::notify("nx_pctl/toast/todo"_i18n);
        return true;
    };

    this->item_refresh   ->registerClickAction(placeholder);
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
