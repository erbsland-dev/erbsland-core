// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../NistPrimeCurve.hpp"

#include "../../../HashAlgorithm.hpp"

namespace erbsland::cryptology::impl::ecdsa_signature {

/// Decoded supported ECDSA algorithm parameters.
/// @tested{EcdsaSignatureTest EcdsaSignatureFullTest}
struct Parameters final {
    NistPrimeCurve::Name curve; ///< Required named curve.
    HashAlgorithm hash;         ///< Required matching SHA-2 algorithm.
};

/// The two nonzero ECDSA signature integers.
/// @tested{EcdsaSignatureTest EcdsaSignatureFullTest}
struct SignatureValues final {
    NistPrimeCurve::Number r; ///< ECDSA signature component r.
    NistPrimeCurve::Number s; ///< ECDSA signature component s.
};

}
