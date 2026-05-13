// numpad — thin wrapper around the system swkbd applet for the play-timer's
// "enter minutes per day" prompts (0..1440 range, NumPad layout).
// Copyright (C) 2026 Taylor.  GPLv3-or-later (see LICENSE).
#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace numpad
{

// Pops the system NumPad applet. Returns the entered minute value (clamped
// 0..1440) when the user confirms; std::nullopt when they cancel or swkbd
// fails to open. Blocks while the applet is up (it overlays our window).
std::optional<uint16_t> prompt_minutes(const std::string& header, uint16_t current);

}   // namespace numpad
