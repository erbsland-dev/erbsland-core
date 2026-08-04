// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::path {

/// The status after the current operation.
enum class PathProgressStatus : uint8_t {
    Success,
    Failed,
};

}
