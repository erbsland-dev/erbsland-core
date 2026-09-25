// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/path/Path.hpp>
#include <erbsland/text/String.hpp>

namespace erbsland::package {

/// One executable selected by the consumer's CMake package call.
/// @notest{Covered by package integration tests.}
struct PackageTarget final {
    text::String name; ///< CMake target name.
    path::Path file;    ///< Built executable path.
    bool bundle{};      ///< Whether CMake built a MACOSX_BUNDLE target.
};

}
