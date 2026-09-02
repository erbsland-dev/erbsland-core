// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "OptionInteger.hpp"

#include "../text/StringList.hpp"

#include <variant>
#include <vector>

namespace erbsland::options {

/// Storage for a parsed option value or default value.
/// Sensitive text is stored as a marked scalar string and never as a list or default value.
/// @tested{OptionsParserTest}
using OptionValueStorage = std::variant<
    std::monostate,
    bool,
    std::vector<bool>,
    OptionInteger,
    std::vector<OptionInteger>,
    text::String,
    text::StringList>;

}
