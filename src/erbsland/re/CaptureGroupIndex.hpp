// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

#include <cstdint>

namespace erbsland::re {

/// The index of a capture group.
///
/// Group index 0 is the full match, 1 is the first capture group, etc.
using CaptureGroupIndex = uint16_t;

}
