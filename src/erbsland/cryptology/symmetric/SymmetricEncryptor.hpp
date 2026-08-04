// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "SymmetricEncryptionType.hpp"
#include "SymmetricIv.hpp"
#include "SymmetricKey.hpp"
#include "SymmetricNonce.hpp"
#include "SymmetricTag.hpp"

#include "../impl/symmetric/SymmetricEncryptorBackendAccess_fwd.hpp"
#include "../impl/symmetric/SymmetricEncryptorData_fwd.hpp"

#include "../../mem/ByteBlock_fwd.hpp"
#include "../../mem/ByteSpan.hpp"

#include <memory>

namespace erbsland::cryptology {

/// A move-only state object for streaming symmetric encryption.
/// A default-constructed or securely erased encryptor is an empty placeholder. Associated data must precede payload
/// data, finalization is allowed once, and AEAD tags are available only after successful finalization.
/// @seedoc{/reference/cryptology/symmetric_encryption}
/// @tested{SymmetricEncryptionFrontendTest}
class SymmetricEncryptor final {
    friend class impl::SymmetricEncryptorBackendAccess;

    /// Track the lifecycle of the encryptor frontend.
    enum class State : uint8_t {
        Empty,     ///< No worker is present.
        Active,    ///< The worker accepts associated or payload data.
        Payload,   ///< Payload processing has started.
        Finalized, ///< Encryption completed successfully.
        Failed,    ///< A backend operation failed.
    };

public:
    /// Create an empty placeholder.
    SymmetricEncryptor() noexcept = default;
    /// Create an AEAD encryptor.
    /// @param type The authenticated encryption construction.
    /// @param key The exact-size secret key.
    /// @param nonce The exact-size nonce.
    /// @throws err::ParameterError If the type or data lengths do not match.
    /// @throws CryptologyError If no backend is available or backend initialization fails.
    SymmetricEncryptor(SymmetricEncryptionType type, const SymmetricKey &key, const SymmetricNonce &nonce);
    /// Create an IV-based encryptor.
    /// @param type The unauthenticated encryption construction.
    /// @param key The exact-size secret key.
    /// @param iv The exact-size initialization vector.
    /// @throws err::ParameterError If the type or data lengths do not match.
    /// @throws CryptologyError If no backend is available or backend initialization fails.
    SymmetricEncryptor(SymmetricEncryptionType type, const SymmetricKey &key, const SymmetricIv &iv);

    /// Securely release the encryption backend state.
    ~SymmetricEncryptor();

    // defaults/deletions
    SymmetricEncryptor(const SymmetricEncryptor &) = delete;
    SymmetricEncryptor(SymmetricEncryptor &&other) noexcept;

    // defaults/deletions
    auto operator=(const SymmetricEncryptor &) -> SymmetricEncryptor & = delete;
    /// Move encryption state into this instance, securely replacing any current state.
    auto operator=(SymmetricEncryptor &&other) noexcept -> SymmetricEncryptor &;

public:
    /// Securely erase the worker, key, nonce or IV, and message state, leaving an empty placeholder.
    void secureErase() noexcept;
    /// Add authenticated associated data.
    /// Empty spans are accepted and do not begin payload processing.
    /// @param data The next associated-data bytes.
    /// @throws err::LogicError If this encryptor is empty, unauthenticated, failed, finalized, or processing payload.
    void addAuthenticatedData(mem::ConstByteSpan data);
    /// Add authenticated associated data from an owning byte block.
    /// @param data The next associated-data bytes.
    /// @throws err::LogicError If this encryptor is empty, unauthenticated, failed, finalized, or processing payload.
    void addAuthenticatedData(const mem::ByteBlock &data);
    /// Encrypt payload bytes.
    /// Empty spans are accepted. CBC workers buffer partial blocks according to their selected padding mode.
    /// @param data The next plaintext bytes.
    /// @return The encrypted output currently available.
    /// @throws err::LogicError If this encryptor is empty, failed, or finalized.
    /// @throws CryptologyError If the backend fails.
    [[nodiscard]] auto encrypt(mem::ConstByteSpan data) -> mem::ByteBlock;
    /// Encrypt payload bytes from an owning byte block.
    /// @param data The next plaintext bytes.
    /// @return The encrypted output currently available.
    /// @throws err::LogicError If this encryptor is empty, failed, or finalized.
    /// @throws CryptologyError If the backend fails.
    [[nodiscard]] auto encrypt(const mem::ByteBlock &data) -> mem::ByteBlock;
    /// Finalize encryption.
    /// @return Any final encrypted output, including selected CBC padding.
    /// @throws err::LogicError If this encryptor is empty, failed, or already finalized.
    /// @throws CryptologyError If the backend fails.
    [[nodiscard]] auto finalize() -> mem::ByteBlock;
    /// Get the AEAD authentication tag after finalization.
    /// @return The cached authentication tag.
    /// @throws err::LogicError If this encryptor is not AEAD or has not finalized successfully.
    [[nodiscard]] auto tag() const -> SymmetricTag;

public: // tests
    /// Test if no encryption worker or secret state is present.
    [[nodiscard]] auto isEmpty() const noexcept -> bool { return _data == nullptr; }

public: // accessors
    /// Get the configured encryption type.
    /// @throws err::LogicError If this encryptor is empty.
    [[nodiscard]] auto type() const -> SymmetricEncryptionType;

private:
    /// Create a facade around a backend worker.
    explicit SymmetricEncryptor(std::unique_ptr<impl::SymmetricEncryptorData> data) noexcept;
    /// Validate the common type and key parameters.
    static void validateCommon(SymmetricEncryptionType type, const SymmetricKey &key);
    /// Validate parameters for an AEAD worker.
    static void validateNonce(SymmetricEncryptionType type, const SymmetricNonce &nonce);
    /// Validate parameters for an IV-based worker.
    static void validateIv(SymmetricEncryptionType type, const SymmetricIv &iv);
    /// Throw if no usable worker is present.
    void requireUsable() const;

private:
    std::unique_ptr<impl::SymmetricEncryptorData> _data; ///< The uniquely owned encryption worker.
    State _state{State::Empty};                          ///< The frontend lifecycle state.
};

}
