// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ProtectedDataAccess_fwd.hpp"
#include "ProtectedDataProvider.hpp"

#include "../../symmetric/SymmetricKey.hpp"
#include "../../symmetric/SymmetricNonce.hpp"

#include <cstdint>
#include <mutex>

namespace erbsland::cryptology::impl {

/// Protect data using AES-256-GCM as specified by NIST SP 800-38D sections 5, 7, and 8.2.1.
/// Each application owns one random key and one serialized 64-bit invocation counter. The authenticated envelope is
/// private to this provider and consists of a 96-bit nonce, a 128-bit tag, and ciphertext.
/// @tested{ProtectedByteBlockTest}
class InternalProtectedDataProvider final : public ProtectedDataProvider {
    friend class ProtectedDataTestAccess;

public:
    /// Generate the application-local AES-256 key in sensitive storage.
    InternalProtectedDataProvider();
    /// Securely erase the application-local AES-256 key.
    ~InternalProtectedDataProvider() override;

    // defaults/deletions
    InternalProtectedDataProvider(const InternalProtectedDataProvider &) = delete;
    InternalProtectedDataProvider(InternalProtectedDataProvider &&) = delete;
    auto operator=(const InternalProtectedDataProvider &) -> InternalProtectedDataProvider & = delete;
    auto operator=(InternalProtectedDataProvider &&) -> InternalProtectedDataProvider & = delete;

public: // implement ProtectedDataProvider
    [[nodiscard]] auto protect(mem::ConstByteSpan plaintext, unit::ByteLength plaintextLength)
        -> mem::ByteBlock override;
    [[nodiscard]] auto unprotect(mem::ConstByteSpan envelope, unit::ByteLength plaintextLength)
        -> mem::ByteBlock override;

private:
    /// Encode the plaintext length as eight big-endian authenticated-data bytes.
    [[nodiscard]] static auto lengthData(unit::ByteLength plaintextLength) -> mem::ByteBlock;
    /// Create the next NIST SP 800-38D section 8.2.1 deterministic nonce for this key.
    [[nodiscard]] auto nextNonce() -> SymmetricNonce;

private:
    static constexpr std::size_t cNonceSize{12U}; ///< 96-bit nonce length recommended by NIST SP 800-38D section 5.2.1.
    static constexpr std::size_t cTagSize{16U};   ///< 128-bit authentication tag selected from section 5.2.1.

    std::mutex _mutex;                            ///< Serialize provider use and nonce allocation.
    SymmetricKey _key;                            ///< Sensitive application-local AES-256 key.
    uint64_t _nonceCounter{0U};                   ///< Last issued invocation counter; zero is never emitted.
};

}
