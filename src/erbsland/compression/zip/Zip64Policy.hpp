// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::compression::zip {

/// Control when writers emit ZIP64 records and sentinel fields.
enum class Zip64Policy : uint8_t {
    Automatic, ///< Emit ZIP64 only where classic fields are insufficient.
    Always,    ///< Emit ZIP64 records even for small archives.
    Never      ///< Reject entries before a ZIP64-only record would be written.
};

}
