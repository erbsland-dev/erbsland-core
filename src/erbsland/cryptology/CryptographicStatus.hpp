// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::cryptology {

/// The current usability status of a cryptographic algorithm.
/// This status is library policy and can change between releases as cryptographic guidance evolves.
/// @seedoc{/reference/cryptology/hashing}
enum class CryptographicStatus : uint8_t {
    Disallowed, ///< Known to be unsuitable for cryptographic use.
    Legacy,     ///< Available only for processing or migrating existing data.
    Acceptable, ///< Suitable for creating new cryptographic results.
};

}
