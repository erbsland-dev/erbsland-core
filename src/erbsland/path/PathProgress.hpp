// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../unit/ElementCount.hpp"

#include <cstdint>
#include <functional>

namespace erbsland::path {

/// The status after the current operation.
enum class PathProgressStatus : uint8_t {
    Success,
    Failed,
};

/// The progress after the current operation.
/// @tested{PathOperationsTest}
struct PathProgress final {
    PathProgressStatus status;    ///< The status after the current operation.
    unit::ElementCount total;     ///< Total paths to process, or infinite if unknown.
    unit::ElementCount processed; ///< Number of paths processed so far.
    unit::ElementCount errors;    ///< Number of errors encountered while ignoring errors.
};

using PathProgressFn = std::function<void(const PathProgress &)>;

}
