// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/String.hpp>

namespace tool::doc::impl {

/// One public symbol and its Sphinx cross-reference role.
struct DoxygenSymbol {
    el::String role; ///< Sphinx C++ role.
    el::String name; ///< Fully qualified target.
};

}
