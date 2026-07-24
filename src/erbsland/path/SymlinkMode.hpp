// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

#include <cstdint>

namespace erbsland::path {

/// The mode to use when a symlink is detected for an operation.
enum class SymlinkMode : uint8_t {
    Follow, ///< Follow the symlink
    Skip,   ///< Skip any symlink.
    Use,    ///< Use the symlink as it is (e.g., copy, access), do not follow it.
};

}
