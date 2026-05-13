// NX-Pctl-Manager — borealis entry point. Step (a) hello-world: init borealis,
// open the window, push MainActivity (which loads activity/main.xml from RomFS).
// Copyright (C) 2026 Taylor.  This program is free software under the GNU
// General Public License v3 or later; it comes with NO WARRANTY. See the
// LICENSE file or <https://www.gnu.org/licenses/gpl-3.0.html> for details.
#include <borealis.hpp>
#include <cstdlib>

#include "activity/main_activity.hpp"

using namespace brls::literals;   // enables the "key"_i18n suffix

int main(int argc, char* argv[])
{
    (void)argc; (void)argv;

    if (!brls::Application::init()) {
        brls::Logger::error("Unable to init borealis Application");
        return EXIT_FAILURE;
    }

    brls::Application::createWindow("nx_pctl/title"_i18n);

    // Auto-follow system theme; user can override via dev settings if needed.
    brls::Application::getPlatform()->setThemeVariant(brls::ThemeVariant::DARK);

    // We provide our own quit path (B on the root activity); no global Start-quit.
    brls::Application::setGlobalQuit(false);

    brls::Application::pushActivity(new MainActivity());

    while (brls::Application::mainLoop())
        ;

    return EXIT_SUCCESS;
}
