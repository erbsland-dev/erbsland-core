// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::text::punycode {

/// Select pure Punycode or strict IDNA2008 processing.
enum class PunycodeMode : uint8_t {
    Pure,           ///< Apply only the RFC 3492 Bootstring algorithm.
    Idna2008Label,  ///< Process one strict IDNA2008 label.
    Idna2008Domain, ///< Process one strict IDNA2008 domain name.
};

}
