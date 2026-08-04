// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "RsaSignatureTypes.hpp"

#include "../../../../mem/SecureErase.hpp"

#include <array>

namespace erbsland::cryptology::impl::rsa_signer::arithmetic {

/// Fixed scratch storage for one maximum-width CIOS Montgomery multiplication.
/// @notest{Storage-only implementation detail exercised by SigningPrivateKeyTest.}
struct RsaMontgomeryScratch final {
    std::array<uint32_t, rsa_signature::cMaximumWords + 2U> words{}; ///< Product and reduction accumulator.

    /// Erase the complete accumulator, including inactive upper words.
    void secureErase() noexcept { mem::secureErase(std::span{words}); }
};

}
