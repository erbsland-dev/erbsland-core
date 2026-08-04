// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::network {

/// Select the semantic Unicode or ASCII transport form of a host name.
enum class HostNameFormat : uint8_t {
    Unicode,  ///< Return canonical lowercase NFC Unicode.
    IdnaAscii ///< Return canonical lowercase IDNA2008 A-label/NR-LDH text.
};

}
