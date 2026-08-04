// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#ifndef __APPLE__
#error "This header is only available on macOS."
#endif

#include "ProtectedDataProvider.hpp"

#include <Security/Security.h>

namespace erbsland::cryptology::impl {

/// Delegate protected data to Security framework ECIES with an ephemeral Secure Enclave P-256 key.
/// The selected primitive is `kSecKeyAlgorithmECIESEncryptionCofactorX963SHA256AESGCM` from Apple's Security API.
/// Source: https://developer.apple.com/documentation/security/protecting-keys-with-the-secure-enclave
/// @notest{Covered by the macOS protected-data integration test.}
class MacosProtectedDataProvider final : public ProtectedDataProvider {
public:
    /// Create an ephemeral Secure Enclave key pair.
    MacosProtectedDataProvider();
    /// Delete and release the application-lifetime key pair.
    ~MacosProtectedDataProvider() override;

    // defaults/deletions
    MacosProtectedDataProvider(const MacosProtectedDataProvider &) = delete;
    MacosProtectedDataProvider(MacosProtectedDataProvider &&) = delete;
    auto operator=(const MacosProtectedDataProvider &) -> MacosProtectedDataProvider & = delete;
    auto operator=(MacosProtectedDataProvider &&) -> MacosProtectedDataProvider & = delete;

public: // implement ProtectedDataProvider
    [[nodiscard]] auto protect(mem::ConstByteSpan plaintext, unit::ByteLength plaintextLength)
        -> mem::ByteBlock override;
    [[nodiscard]] auto unprotect(mem::ConstByteSpan envelope, unit::ByteLength plaintextLength)
        -> mem::ByteBlock override;

private:
    /// Throw a cryptology error and release optional Core Foundation error details.
    [[noreturn]] static void throwError(CFErrorRef error);

private:
    CFDataRef _applicationTag{nullptr}; ///< Random keychain tag used to delete the application-lifetime key.
    SecKeyRef _privateKey{nullptr};     ///< Application-lifetime Secure Enclave private key.
    SecKeyRef _publicKey{nullptr};      ///< Matching public key used for encryption.
};

}
