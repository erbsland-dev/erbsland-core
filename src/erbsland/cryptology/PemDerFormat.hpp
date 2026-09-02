// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::cryptology {

/// The binary or textual representation of a DER-based cryptographic artifact.
enum class PemDerFormat : uint8_t {
    Automatic, ///< Select from a recognized artifact-specific suffix or input content.
    Pem,       ///< RFC 7468 textual representation.
    Der,       ///< Canonical DER representation.
};

}
