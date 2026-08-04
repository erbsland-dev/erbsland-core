// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <string_view>

namespace erbsland::util::impl {

/// Throw an out-of-range error without introducing an exception-header dependency into container templates.
/// @param reason The reason for the error.
/// @throws err::OutOfRangeError Always.
/// @tested{ListTest}
[[noreturn]] void throwOutOfRange(std::string_view reason);

}
