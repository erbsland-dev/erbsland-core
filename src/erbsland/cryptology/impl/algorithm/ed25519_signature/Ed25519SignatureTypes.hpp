// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "FieldElement.hpp"

#include <array>
#include <cstdint>

namespace erbsland::cryptology::impl::ed25519_signature {

/// One RFC 8032 section 5.1.4 extended homogeneous point `(X,Y,Z,T)`.
/// @tested{Ed25519SignatureTest Ed25519SignatureFullTest}
struct Point final {
    FieldElement x; ///< Homogeneous x numerator.
    FieldElement y; ///< Homogeneous y numerator.
    FieldElement z; ///< Common homogeneous denominator.
    FieldElement t; ///< Product satisfying `x*y = t*z`.

    /// Erase every projective coordinate.
    void secureErase() noexcept {
        x.secureErase();
        y.secureErase();
        z.secureErase();
        t.secureErase();
    }
};

/// One scalar modulo the RFC 8032 Ed25519 subgroup order L.
using Scalar = std::array<uint32_t, 8U>;

}
