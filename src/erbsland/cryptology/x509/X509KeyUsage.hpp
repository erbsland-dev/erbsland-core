// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../util/EnumFlags.hpp"

#include <cstdint>

namespace erbsland::cryptology {

/// X.509 Key Usage bits.
enum class X509KeyUsage : uint16_t {
    None = 0U,                     ///< No decoded usage bits.
    DigitalSignature = 1U << 0U,   ///< Digital signatures other than certificate or CRL signatures.
    ContentCommitment = 1U << 1U,  ///< Non-repudiation or content commitment.
    KeyEncipherment = 1U << 2U,    ///< Key transport.
    DataEncipherment = 1U << 3U,   ///< Direct data encipherment.
    KeyAgreement = 1U << 4U,       ///< Key agreement.
    KeyCertificateSign = 1U << 5U, ///< Certificate signing.
    CrlSign = 1U << 6U,            ///< CRL signing.
    EncipherOnly = 1U << 7U,       ///< Encipher-only key agreement.
    DecipherOnly = 1U << 8U,       ///< Decipher-only key agreement.
};

/// A set of X.509 Key Usage bits.
using X509KeyUsages = util::EnumFlags<X509KeyUsage>;

}
