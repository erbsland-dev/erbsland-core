// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../text/StringViewList.hpp"

namespace erbsland::core {

/// A list of command line arguments.
/// @tested{ApplicationOptionsTest OptionsUsageTest}
using CommandLineArguments = text::StringViewList;

}
