// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "OptionInteger.hpp"

#include "../text/String.hpp"

#include <variant>
#include <vector>

namespace erbsland::options {

/// Storage for a parsed option value or default value.
/// @tested{OptionsFrameworkTest}
using OptionValueStorage =
    std::variant<bool, OptionInteger, std::vector<OptionInteger>, text::String, std::vector<text::String>>;

}
