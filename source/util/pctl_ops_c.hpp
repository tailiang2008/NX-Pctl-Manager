// extern "C" bridge so the (unmodified) C service layer in source/pctl_ops.{h,c}
// is callable from our C++ UI code.  Include this anywhere you'd want pctl_ops.h.
// Copyright (C) 2026 Taylor.  GPLv3-or-later (see LICENSE).
#pragma once

extern "C" {
#include "../pctl_ops.h"
}
