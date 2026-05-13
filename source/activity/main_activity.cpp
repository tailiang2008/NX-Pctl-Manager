// Copyright (C) 2026 Taylor.  GPLv3-or-later (see LICENSE).
#include "activity/main_activity.hpp"

#include <fmt/format.h>

#include "app.hpp"
#include "util/pctl_ops_c.hpp"

using namespace brls::literals;   // for "key"_i18n

namespace
{
// Common shape for the two destructive cells (delete-all / unlink):
//   show a confirm Dialog → on confirm run `action` → toast result → refresh the panel.
// `error_prefix` is the lead text for failure ("Could not X"); the Result code is appended.
template <typename Action, typename Panel>
void run_destructive(const std::string& body, const std::string& confirm_label,
                     const std::string& success_toast, const std::string& error_prefix,
                     Action action, Panel* panel)
{
    auto* dialog = new brls::Dialog(body);
    dialog->addButton("hints/cancel"_i18n, []() {});
    dialog->addButton(confirm_label, [=]() {
        Result rc = action();
        if (R_SUCCEEDED(rc))
            brls::Application::notify(success_toast);
        else
            brls::Application::notify(
                fmt::format("{} (error 0x{:08X}).", error_prefix, (unsigned)rc));
        panel->refresh();
    });
    dialog->open();
}
}   // namespace

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

    // Delete all parental controls (cmd 1043) — step (d), destructive, confirm-gated.
    this->item_delete_pc->registerClickAction([this](brls::View*) {
        run_destructive(
            "nx_pctl/dialog/delete_pc/body"_i18n,
            "nx_pctl/dialog/delete_pc/confirm"_i18n,
            "nx_pctl/toast/delete_pc_ok"_i18n,
            "nx_pctl/toast/delete_pc_err"_i18n,
            pctl_delete_parental_controls,
            this->status_panel.getView());
        return true;
    });

    // Unlink companion app (cmd 1941) — step (d), destructive, confirm-gated.
    this->item_unlink->registerClickAction([this](brls::View*) {
        run_destructive(
            "nx_pctl/dialog/unlink/body"_i18n,
            "nx_pctl/dialog/unlink/confirm"_i18n,
            "nx_pctl/toast/unlink_ok"_i18n,
            "nx_pctl/toast/unlink_err"_i18n,
            pctl_delete_pairing,
            this->status_panel.getView());
        return true;
    });

    // The two remaining cells still toast a placeholder until (e) / (f).
    auto placeholder = [](brls::View*) {
        brls::Application::notify("nx_pctl/toast/todo"_i18n);
        return true;
    };
    this->item_play_timer->registerClickAction(placeholder);
    this->item_set_pin   ->registerClickAction(placeholder);

    // First read of the status panel — onResume runs it again on every entry,
    // but a freshly created Activity doesn't get an onResume call.
    this->status_panel->refresh();
}

void MainActivity::onResume()
{
    brls::Activity::onResume();
    this->status_panel->refresh();
}
