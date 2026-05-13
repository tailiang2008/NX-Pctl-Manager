// Copyright (C) 2026 Taylor.  GPLv3-or-later (see LICENSE).
#include "util/numpad.hpp"

#include <cstdio>
#include <cstdlib>

#include <switch.h>   // SwkbdConfig / swkbdShow — libnx headers carry their own extern "C"

namespace numpad
{

std::optional<uint16_t> prompt_minutes(const std::string& header, uint16_t current)
{
    SwkbdConfig kbd;
    if (R_FAILED(swkbdCreate(&kbd, 0))) return std::nullopt;

    swkbdConfigMakePresetDefault(&kbd);
    swkbdConfigSetType(&kbd, SwkbdType_NumPad);
    swkbdConfigSetStringLenMin(&kbd, 1);
    swkbdConfigSetStringLenMax(&kbd, 4);
    swkbdConfigSetHeaderText(&kbd, header.c_str());
    swkbdConfigSetGuideText(&kbd, "minutes per day  (0 = no play that day; max 1440)");

    char initial[8];
    std::snprintf(initial, sizeof(initial), "%u", (unsigned)current);
    swkbdConfigSetInitialText(&kbd, initial);

    char out[16] = {0};
    Result rc    = swkbdShow(&kbd, out, sizeof(out));
    swkbdClose(&kbd);
    if (R_FAILED(rc)) return std::nullopt;   // cancelled

    long v = std::strtol(out, nullptr, 10);
    if (v < 0)    v = 0;
    if (v > 1440) v = 1440;
    return (uint16_t)v;
}

}   // namespace numpad
