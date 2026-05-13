// app — process-wide pctl lifecycle helpers. Wraps pctl_ops_init/exit and
// remembers whether init succeeded; views that read state should ask
// app::pctl_available() first so they degrade gracefully (e.g. on a non-CFW
// console where pctlInitialize returns an error).
// Copyright (C) 2026 Taylor.  GPLv3-or-later (see LICENSE).
#pragma once

#include <cstdint>   // for uint32_t — used by pctl_init_result()

namespace app
{

// Open the pctl service. Returns true on success; on failure, pctl_init_result()
// holds the Result so the UI can show the user a meaningful error.
bool init();

void shutdown();

bool        pctl_available();
uint32_t    pctl_init_result();   // 0 if init succeeded

}   // namespace app
