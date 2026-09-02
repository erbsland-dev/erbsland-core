// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../util/EnumFlags.hpp"

#include <cstdint>

namespace erbsland::options {

/// Flags that customize the command-line parser.
enum class OptionParserFlag : uint8_t {
    None = 0,                                   ///< No flags.
    DisableHelp = 1U << 0U,                     ///< Disable the built-in `-h` and `--help` request.
    DisableVersion = 1U << 1U,                  ///< Disable the built-in `--version` request.
    NoHelpDetails = 1U << 2U,                   ///< Disable detailed help and treat `--help` as a pure flag.
    ErrorOnEmptyRequiredPositionals = 1U << 3U, ///< Report missing required positional arguments on an empty command.
    ErrorOnMissingModule = 1U << 4U,            ///< Report a missing module instead of displaying the module overview.
    All = 0b1'1111U,                            ///< All flags.
};

/// A set of command-line parser flags.
using OptionParserFlags = util::EnumFlags<OptionParserFlag>;

/// Combine two command-line parser flags.
[[nodiscard]] constexpr auto operator|(OptionParserFlag left, OptionParserFlag right) noexcept -> OptionParserFlags {
    return OptionParserFlags{left} | right;
}

}
