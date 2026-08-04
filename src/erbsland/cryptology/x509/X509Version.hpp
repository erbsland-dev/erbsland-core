// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::cryptology {

/// An X.509 certificate version.
enum class X509Version : uint8_t {
    Unknown, ///< The certificate version was not recognized.
    V1,      ///< X.509 version 1.
    V2,      ///< X.509 version 2.
    V3,      ///< X.509 version 3.
};

}
