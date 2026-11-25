// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/Path.hpp>

namespace tool::elcl_format::impl {

/// Copy permission bits without changing the original file.
/// @param original Existing original file.
/// @param temporary Completed replacement file.
void copyFilePermissions(const el::Path &original, const el::Path &temporary);

}
