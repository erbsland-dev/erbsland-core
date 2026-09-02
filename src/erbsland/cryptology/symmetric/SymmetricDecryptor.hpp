// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "SymmetricEncryptionType.hpp"
#include "SymmetricIv.hpp"
#include "SymmetricKey.hpp"
#include "SymmetricNonce.hpp"
#include "SymmetricTag.hpp"

#include "../impl/symmetric/SymmetricDecryptorBackendAccess_fwd.hpp"
#include "../impl/symmetric/SymmetricDecryptorData_fwd.hpp"

#include "../../mem/ByteBlock_fwd.hpp"
#include "../../mem/ByteSpan.hpp"

#include <memory>

namespace erbsland::cryptology {

/// A move-only state object for streaming symmetric decryption.
/// A default-constructed or securely erased decryptor is an empty placeholder. AEAD plaintext returned by `decrypt()`
/// is unauthenticated and must not be trusted or acted upon until `finalize(tag)` succeeds.
/// @seedoc{/reference/cryptology/cryptographic_operations}
/// @tested{SymmetricEncryptionFrontendTest}
class SymmetricDecryptor final {
    friend class impl::SymmetricDecryptorBackendAccess;

    /// Track the lifecycle of the decryptor frontend.
    enum class State : uint8_t {
        Empty,     ///< No worker is present.
        Active,    ///< The worker accepts associated or payload data.
        Payload,   ///< Payload processing has started.
        Finalized, ///< Decryption completed successfully.
        Failed,    ///< A backend operation failed.
    };

public:
    /// Create an empty placeholder.
    SymmetricDecryptor() noexcept = default;
    /// Create an AEAD decryptor.
    /// @param type The authenticated encryption construction.
    /// @param key The exact-size secret key.
    /// @param nonce The exact-size nonce.
    /// @throws err::ParameterError If the type or data lengths do not match.
    /// @throws CryptologyError If no backend is available or backend initialization fails.
    SymmetricDecryptor(SymmetricEncryptionType type, const SymmetricKey &key, const SymmetricNonce &nonce);
    /// Create an IV-based decryptor.
    /// @param type The unauthenticated encryption construction.
    /// @param key The exact-size secret key.
    /// @param iv The exact-size initialization vector.
    /// @throws err::ParameterError If the type or data lengths do not match.
    /// @throws CryptologyError If no backend is available or backend initialization fails.
    SymmetricDecryptor(SymmetricEncryptionType type, const SymmetricKey &key, const SymmetricIv &iv);

    /// Release the backend and securely erase retained cryptographic state.
    ~SymmetricDecryptor();

    // defaults/deletions
    SymmetricDecryptor(const SymmetricDecryptor &) = delete;
    SymmetricDecryptor(SymmetricDecryptor &&other) noexcept;

    // defaults/deletions
    auto operator=(const SymmetricDecryptor &) -> SymmetricDecryptor & = delete;
    /// Move decryption state into this instance, securely replacing any current state.
    auto operator=(SymmetricDecryptor &&other) noexcept -> SymmetricDecryptor &;

public:
    /// Securely erase the worker, key, nonce or IV, and message state, leaving an empty placeholder.
    void secureErase() noexcept;
    /// Add authenticated associated data.
    /// Empty spans are accepted and do not begin payload processing.
    /// @param data The next associated-data bytes.
    /// @throws err::LogicError If this decryptor is empty, unauthenticated, failed, finalized, or processing payload.
    void addAuthenticatedData(mem::ConstByteSpan data);
    /// Add authenticated associated data from an owning byte block.
    /// @param data The next associated-data bytes.
    /// @throws err::LogicError If this decryptor is empty, unauthenticated, failed, finalized, or processing payload.
    void addAuthenticatedData(const mem::ByteBlock &data);
    /// Decrypt payload bytes.
    /// For AEAD types, returned plaintext is unauthenticated until `finalize(tag)` succeeds.
    /// @param data The next encrypted bytes.
    /// @return The sensitive-marked plaintext currently available.
    /// @throws err::LogicError If this decryptor is empty, failed, or finalized.
    /// @throws CryptologyError If the backend rejects malformed encrypted data or otherwise fails.
    [[nodiscard]] auto decrypt(mem::ConstByteSpan data) -> mem::ByteBlock;
    /// Decrypt payload bytes from an owning byte block.
    /// For AEAD types, returned plaintext is unauthenticated until `finalize(tag)` succeeds.
    /// @param data The next encrypted bytes.
    /// @return The sensitive-marked plaintext currently available.
    /// @throws err::LogicError If this decryptor is empty, failed, or finalized.
    /// @throws CryptologyError If the backend rejects malformed encrypted data or otherwise fails.
    [[nodiscard]] auto decrypt(const mem::ByteBlock &data) -> mem::ByteBlock;
    /// Finalize and authenticate AEAD decryption.
    /// Previously returned plaintext becomes authenticated only when this call succeeds.
    /// @param tag The exact-size expected authentication tag.
    /// @return Any sensitive-marked final plaintext output.
    /// @throws err::ParameterError If the tag length does not match the selected type.
    /// @throws err::LogicError If this decryptor is unauthenticated, empty, failed, or already finalized.
    /// @throws CryptologyError If authentication or the backend fails.
    [[nodiscard]] auto finalize(const SymmetricTag &tag) -> mem::ByteBlock;
    /// Finalize unauthenticated CBC decryption and remove padding when the selected mode defines it.
    /// Random-fill CBC cannot remove its final fill bytes; the caller must crop the output using the externally stored
    /// original length.
    /// @return Any sensitive-marked final plaintext output.
    /// @throws err::LogicError If this decryptor is AEAD, empty, failed, or already finalized.
    /// @throws CryptologyError If ciphertext alignment, padding, or the backend is invalid.
    [[nodiscard]] auto finalize() -> mem::ByteBlock;

public: // tests
    /// Test if no decryption worker or secret state is present.
    [[nodiscard]] auto isEmpty() const noexcept -> bool { return _data == nullptr; }

public: // accessors
    /// Get the configured encryption type.
    /// @throws err::LogicError If this decryptor is empty.
    [[nodiscard]] auto type() const -> SymmetricEncryptionType;

private:
    /// Create a facade around a backend worker.
    explicit SymmetricDecryptor(std::unique_ptr<impl::SymmetricDecryptorData> data) noexcept;
    /// Validate the common type and key parameters.
    static void validateCommon(SymmetricEncryptionType type, const SymmetricKey &key);
    /// Validate parameters for an AEAD worker.
    static void validateNonce(SymmetricEncryptionType type, const SymmetricNonce &nonce);
    /// Validate parameters for an IV-based worker.
    static void validateIv(SymmetricEncryptionType type, const SymmetricIv &iv);
    /// Mark a plaintext result as sensitive.
    [[nodiscard]] static auto markSensitive(mem::ByteBlock result) noexcept -> mem::ByteBlock;
    /// Throw if no usable worker is present.
    void requireUsable() const;

private:
    std::unique_ptr<impl::SymmetricDecryptorData> _data; ///< The uniquely owned decryption worker.
    State _state{State::Empty};                          ///< The frontend lifecycle state.
};

}
