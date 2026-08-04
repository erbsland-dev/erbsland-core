// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/core/MakeOneNamespace.hpp>
#include <erbsland/path/Path.hpp>

namespace demo {

/// The resolved paths for the separate pepper and password-hash database files.
/// @notest{Compiled and exercised as part of the Password Handler demo.}
struct StoragePaths {
    el::Path pepper;   ///< The separate password-hash key file.
    el::Path database; ///< The user database containing only canonical hash records.
};

}
