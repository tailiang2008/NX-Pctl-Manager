// pt_flow — the "write-before-temporary-unlock" gate, ported from main.c's
// pt_ready_to_write. If the play timer is currently active, writing a new
// limit while it's running can lock the console and destabilise Atmosphère —
// so we ask the user, then run cmd 1201 (UnlockRestrictionTemporarily) to
// turn parental controls off, then let the caller do the write.
//
// Asynchronous on purpose: the borealis Dialog runs after a button-tap, so
// the gate returns its decision via a callback rather than blocking.
// Copyright (C) 2026 Taylor.  GPLv3-or-later (see LICENSE).
#pragma once

#include <functional>

namespace pt_flow
{

// Calls `on_ready(true, did_unlock)` when it's safe to write the play timer:
//   - did_unlock == false: timer wasn't active; the caller writes directly,
//     no follow-up notice needed.
//   - did_unlock == true:  parental controls were temporarily turned off via
//     cmd 1201; the caller writes, then should show the "Parental controls
//     are temporarily off — return to the main menu …" dialog.
// Calls `on_ready(false, false)` if the user cancelled or 1201 failed; in
// that case the caller MUST NOT write anything (a toast for the failure is
// already shown by the gate itself).
void ready_to_write(std::function<void(bool ok, bool did_unlock)> on_ready);

}   // namespace pt_flow
