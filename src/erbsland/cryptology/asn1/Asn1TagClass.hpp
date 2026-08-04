// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::cryptology {

/// The ASN.1 tag class encoded in an identifier octet.
enum class Asn1TagClass : uint8_t {
    Universal,   ///< A type defined by ASN.1.
    Application, ///< An application-specific type.
    Context,     ///< A context-specific type.
    Private,     ///< A private type.
};

}
