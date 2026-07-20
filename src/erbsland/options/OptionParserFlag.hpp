// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../util/EnumFlags.hpp"

#include <cstdint>

namespace erbsland::options {

/// Flags that customize the command-line parser.
enum class OptionParserFlag : uint8_t {
    DisableHelp = 1U << 0U,    ///< Disable the built-in `-h` and `--help` request.
    DisableVersion = 1U << 1U, ///< Disable the built-in `--version` request.
    All = (1U << 0U) | (1U << 1U),
};

/// A set of command-line parser flags.
using OptionParserFlags = util::EnumFlags<OptionParserFlag>;

/// Combine two command-line parser flags.
[[nodiscard]] constexpr auto operator|(OptionParserFlag left, OptionParserFlag right) noexcept -> OptionParserFlags {
    return OptionParserFlags{left} | right;
}

}
