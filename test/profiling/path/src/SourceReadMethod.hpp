// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace app::path {

/// The whole-file read API used during a source-tree walk.
enum class SourceReadMethod : std::uint8_t {
    Text, ///< Read the complete file as tolerant UTF-8 text.
    Data, ///< Read the complete file as bytes.
};

}
