// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PathProgressStatus.hpp"

#include "../unit/ItemCount.hpp"

#include <cstdint>
#include <functional>

namespace erbsland::path {

/// The progress after the current operation.
/// @tested{PathOperationsTest}
struct PathProgress final {
    PathProgressStatus status; ///< The status after the current operation.
    unit::ItemCount total;     ///< Total paths to process, or infinite if unknown.
    unit::ItemCount processed; ///< Number of paths processed so far.
    unit::ItemCount errors;    ///< Number of errors encountered while ignoring errors.
};

using PathProgressFn = std::function<void(const PathProgress &)>;

}
