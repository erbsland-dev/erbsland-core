// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::mem {

/// The method used to construct elements in shared array storage.
enum class SharedArrayDataConstructMethod : uint8_t {
    None,             ///< Leave raw trivially copyable element storage unconstructed.
    DefaultConstruct, ///< Default-construct every capacity element.
    ValueConstruct,   ///< Value-construct every capacity element.
};

}
