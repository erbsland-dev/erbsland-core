// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::network::impl {

/// A protocol transition that must be surfaced before processing further peer input.
enum class TlsClientProtocolCheckpoint : uint8_t {
    None,               ///< No checkpoint is pending.
    PeerHello,          ///< EncryptedExtensions was parsed and negotiated parameters are available.
    PeerAuthenticated,  ///< Certificate validation and CertificateVerify succeeded.
    HandshakeCompleted, ///< Server Finished was verified and the client Finished flight was queued.
};

}
