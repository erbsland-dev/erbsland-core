// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../unit/ExitCode.hpp"

#include <functional>

namespace erbsland::core {

/// A main function override for an application.
/// @tested{OptionsFrameworkTest}
using MainFn = std::function<unit::ExitCode()>;

}
