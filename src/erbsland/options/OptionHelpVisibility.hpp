// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

#include <cstdint>

namespace erbsland::options {

/// The help visibility of an option, option set, module, choice, or options root.
/// Visibility only affects generated help documents. It does not disable parsing; use `OptionFlag::Disabled` to
/// remove an option or option set from parsing.
enum class OptionHelpVisibility : uint8_t {
    Inherit,  ///< Inherit visibility from the owning option set, or use `Normal` if no owner defines visibility.
    Hidden,   ///< Accept the option while hiding it from generated help output.
    Normal,   ///< Show in help for the active root or module. This is the default effective visibility.
    Overview, ///< Also show in root overview help when modules exist.
    Usage     ///< Show like `Overview` and render the option explicitly in the usage line.
};

}
