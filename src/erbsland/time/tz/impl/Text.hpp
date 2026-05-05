// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::time::tz::impl {

/// A generated text table entry.
/// @notest{Internal generated-data helper.}
struct Text final {
    uint16_t offset;
    uint8_t length;
};

}
