// NX-Pctl-Manager — borealis entry point. Registers the custom StatusPanel
// XML view, opens the pctl service, and pushes MainActivity onto the stack.
// Copyright (C) 2026 Taylor.  This program is free software under the GNU
// General Public License v3 or later; it comes with NO WARRANTY. See the
// LICENSE file or <https://www.gnu.org/licenses/gpl-3.0.html> for details.
#include <borealis.hpp>
#include <cstdlib>

#include "app.hpp"
#include "activity/main_activity.hpp"
#include "view/pt_state_header.hpp"
#include "view/status_panel.hpp"

using namespace brls::literals;   // enables the "key"_i18n suffix

int main(int argc, char* argv[])
{
    (void)argc; (void)argv;

    if (!brls::Application::init()) {
        brls::Logger::error("Unable to init borealis Application");
        return EXIT_FAILURE;
    }

    brls::Application::createWindow("nx_pctl/title"_i18n);

    // Auto-follow system theme; user can override in dev settings if they want.
    brls::Application::getPlatform()->setThemeVariant(brls::ThemeVariant::DARK);

    // We own our own quit path (B on the root activity); no global Start-quit.
    brls::Application::setGlobalQuit(false);

    // Register custom views referenced from XML by their unprefixed tag name.
    brls::Application::registerXMLView("StatusPanel",   StatusPanel::create);
    brls::Application::registerXMLView("PtStateHeader", PtStateHeader::create);

    // Open the pctl service. On a non-CFW console this returns an error; we
    // still push MainActivity, but the status panel will read "(unavailable)"
    // everywhere and any wired action will surface the error (step (f) adds a
    // dedicated InitErrorActivity for a cleaner first-launch experience).
    if (!app::init()) {
        brls::Logger::error("pctl_ops_init failed (0x{:08X}) — running in "
                            "degraded mode (no CFW?)", app::pctl_init_result());
    }

    brls::Application::pushActivity(new MainActivity());

    while (brls::Application::mainLoop())
        ;

    app::shutdown();
    return EXIT_SUCCESS;
}
