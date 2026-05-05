// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../text/String.hpp"

#include <vector>

namespace erbsland::core {

/// A list of command line arguments.
/// @tested{OptionsFrameworkTest OptionsUsageTest}
using CommandLineArguments = std::vector<text::String>;

}
