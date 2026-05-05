// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::options {

/// The help visibility of an option or option group.
/// In case of conflicts, lower visibility wins.
/// @tested{OptionsFrameworkTest}
enum class OptionHelpVisibility : uint8_t {
    Inherit,  ///< Inherit from the owning set or module, or use main visibility by default.
    Hidden,   ///< Accept, but hide from help.
    Detail,   ///< Only show in detailed help.
    Main,     ///< Show in main help.
    Important ///< Show in main help and on the usage line.
};

}
