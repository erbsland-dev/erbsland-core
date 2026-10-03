// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../text/StringList.hpp"
#include "../../SubprocessOptions.hpp"

namespace erbsland::system::impl {

/// Validate shared owned/detached launch inputs before native process creation.
/// @param executable The direct executable path.
/// @param arguments The argument list.
/// @param options The child configuration.
/// @param detached Whether capture must be rejected.
/// @tested{SubprocessInteropTest SubprocessEventTest}
void validateSubprocess(
    const path::Path &executable, const text::StringList &arguments, const SubprocessOptions &options, bool detached);

}
