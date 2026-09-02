// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HashAlgorithm.hpp"

#include "impl/authentication/HmacWorker_fwd.hpp"

#include "../mem/ByteBlock_fwd.hpp"
#include "../mem/ByteSpan.hpp"
#include "../text/String_fwd.hpp"

#include <memory>

namespace erbsland::cryptology {

/// A move-only streaming RFC 2104 HMAC state for SHA-256 or SHA-384.
/// A default-constructed, moved-from, or securely erased instance is an invalid placeholder. The complete
/// authenticator is cached after finalization; use `reset()` to authenticate another message with the same key.
/// The construction follows RFC 2104 section 2; verification deliberately accepts only the complete hash-sized value.
/// Specification: https://www.rfc-editor.org/rfc/rfc2104.html#section-2
/// @seedoc{/reference/cryptology/cryptographic_operations}
/// @tested{HmacTest HashPrimitiveFullValidationTest}
class Hmac final {
public:
    /// Create an invalid placeholder.
    Hmac() noexcept;
    /// Create keyed HMAC state, sharing and marking the key allocation as sensitive.
    /// HMAC accepts keys of any length, although weak keys are unsuitable for secure applications.
    /// @param algorithm SHA-256 or SHA-384.
    /// @param key The exact secret key bytes.
    /// @throws err::ParameterError If the algorithm is unsupported.
    Hmac(HashAlgorithm algorithm, mem::ByteBlock key);
    /// Create keyed HMAC state by copying the key into sensitive storage.
    /// @param algorithm SHA-256 or SHA-384.
    /// @param key The exact secret key bytes.
    /// @throws err::ParameterError If the algorithm is unsupported.
    Hmac(HashAlgorithm algorithm, mem::ConstByteSpan key);

    /// Securely erase keyed HMAC state.
    ~Hmac();

    // defaults/deletions
    Hmac(const Hmac &) = delete;
    Hmac(Hmac &&other) noexcept;

    // defaults/deletions
    auto operator=(const Hmac &) -> Hmac & = delete;
    /// Move keyed HMAC state into this instance, securely replacing any current state.
    auto operator=(Hmac &&other) noexcept -> Hmac &;

public:
    /// Reset the message while retaining the same algorithm and key.
    /// @throws err::LogicError If this is an invalid placeholder.
    void reset();
    /// Securely erase the key and message state, leaving an invalid placeholder.
    void secureErase() noexcept;
    /// Add exact message bytes.
    /// Empty spans are accepted.
    /// @param data The next authenticated message bytes.
    /// @throws err::LogicError If this state is invalid or finalized.
    /// @throws err::ParameterError If the hash message-length limit would be exceeded.
    void update(mem::ConstByteSpan data);
    /// Add a byte block.
    /// @param data The next authenticated message bytes.
    /// @throws err::LogicError If this state is invalid or finalized.
    /// @throws err::ParameterError If the hash message-length limit would be exceeded.
    void update(const mem::ByteBlock &data);
    /// Add the exact bytes in a UTF-8 string's internal buffer.
    /// @param text The UTF-8 text whose stored bytes are authenticated.
    /// @throws err::LogicError If this state is invalid or finalized.
    /// @throws err::ParameterError If the hash message-length limit would be exceeded.
    void update(const text::String &text);
    /// Finalize the message or return the cached complete authenticator.
    /// @return The full hash-sized HMAC value in ordinary storage.
    /// @throws err::LogicError If this is an invalid placeholder.
    [[nodiscard]] auto finalize() -> mem::ByteBlock;
    /// Verify a complete authenticator without content-dependent short-circuiting.
    /// A length mismatch returns immediately with `false`.
    /// @param expected The expected full hash-sized authenticator.
    /// @return `true` if the complete authenticator matches.
    /// @throws err::LogicError If this is an invalid placeholder.
    [[nodiscard]] auto verify(mem::ConstByteSpan expected) -> bool;
    /// Verify a complete authenticator in an owning byte block.
    /// @param expected The expected full hash-sized authenticator.
    /// @return `true` if the complete authenticator matches.
    /// @throws err::LogicError If this is an invalid placeholder.
    [[nodiscard]] auto verify(const mem::ByteBlock &expected) -> bool;

public: // tests
    /// Test if keyed worker state is present.
    [[nodiscard]] auto isValid() const noexcept -> bool { return _worker != nullptr; }

public: // accessors
    /// Get the underlying hash algorithm.
    /// @throws err::LogicError If this is an invalid placeholder.
    [[nodiscard]] auto algorithm() const -> HashAlgorithm;

private:
    /// Throw if this is an invalid placeholder.
    void requireValid() const;

private:
    std::unique_ptr<impl::HmacWorker> _worker; ///< The uniquely owned keyed worker.
};

}
