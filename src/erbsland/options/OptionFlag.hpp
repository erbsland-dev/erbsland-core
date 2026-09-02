// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../util/EnumFlags.hpp"

#include <cstdint>

namespace erbsland::options {

/// Flags for an option or option set.
///
/// Flags modify parser behavior. Use help visibility to hide enabled options from generated help without disabling
/// parsing.
enum class OptionFlag : uint8_t {
    None = 0U,               ///< No flag.
    Disabled = 1U << 0U,     ///< Do not accept or show the option.
    Required = 1U << 1U,     ///< Require the option to be present.
    Greedy = 1U << 2U,       ///< Allow a positional list option to consume values greedily.
    AcceptAsFlag = 1U << 3U, ///< Allow a named value option to be used without a value.
};

/// A set of option flags.
using OptionFlags = util::EnumFlags<OptionFlag>;

}
