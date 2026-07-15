// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CodeLocation.hpp"

namespace erbsland::unit {

/// A continuous source code range with an exclusive end location.
/// @tested{IntegerUnitTest}
struct CodeContinuousRange final {
    CodeLocation begin; ///< The inclusive begin location.
    CodeLocation end;   ///< The exclusive end location.
};

}
