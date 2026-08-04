// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../TimeAmounts.hpp"
#include "../TimeEpoch.hpp"

namespace erbsland::time::impl {

/// Get the offset between an epoch and the Core epoch in seconds.
[[nodiscard]] auto secondsSinceCoreEpoch(TimeEpoch epoch) noexcept -> Seconds;

}
