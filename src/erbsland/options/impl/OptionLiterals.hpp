// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../text/Literals.hpp"

/// Canonical syntax literals for command-line option processing.
namespace erbsland::options::impl::option_literal {

using namespace text::literals;

inline constexpr auto cHelpShortName = "-h"_el;          ///< Short built-in help name.
inline constexpr auto cHelpLongName = "--help"_el;       ///< Long built-in help name.
inline constexpr auto cVersionLongName = "--version"_el; ///< Built-in version name.
inline constexpr auto cLongNamePrefix = "--"_el;         ///< Long option prefix and end-of-options marker.
inline constexpr auto cEndOfOptions = cLongNamePrefix;   ///< End-of-options marker.
inline constexpr auto cShortNamePrefix = "-"_el;         ///< Short option prefix.
inline constexpr auto cHelpPathSeparator = "/"_el;       ///< Qualified option/choice help separator.
inline constexpr auto cValueSeparator = "="_el;          ///< Attached option-value separator.

}
