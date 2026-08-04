// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../impl/protected_data/ProtectedDataAccess_fwd.hpp"

#include "../../mem/ByteBlock.hpp"
#include "../../mem/ByteSpan.hpp"
#include "../../unit/ByteLength.hpp"

#include <functional>

namespace erbsland::cryptology {

/// An opaque byte block encrypted by the current application's protected-data provider.
/// This type stores no key or provider reference. It is intended as application-lifetime memory hardening, not as a
/// persistent encrypted format or an authorization boundary against code executing inside the process.
/// @seedoc{/reference/cryptology/key_agreement}
/// @tested{ProtectedByteBlockTest}
class ProtectedByteBlock final {
    friend class impl::ProtectedDataTestAccess;

public:
    /// A callback receiving a temporary unprotected byte view.
    using UnprotectedDataFn = std::function<void(mem::ConstByteSpan)>;

public:
    /// Create an empty protected block without accessing the application.
    ProtectedByteBlock() noexcept = default;
    /// Protect a copy of the supplied bytes with the current application provider.
    /// @param data The plaintext bytes to protect. An empty span creates an empty block.
    /// @throws CryptologyError If provider initialization or encryption fails.
    explicit ProtectedByteBlock(mem::ConstByteSpan data);
    /// Protect an owning byte block with the current application provider.
    /// @param data The plaintext bytes to protect. An empty block creates an empty protected block.
    /// @throws CryptologyError If provider initialization or encryption fails.
    explicit ProtectedByteBlock(const mem::ByteBlock &data);
    /// Move a protected byte block
    ProtectedByteBlock(ProtectedByteBlock &&other) noexcept;
    /// Move a protected byte block
    auto operator=(ProtectedByteBlock &&other) noexcept -> ProtectedByteBlock &;

    // defaults
    ~ProtectedByteBlock() = default;
    ProtectedByteBlock(const ProtectedByteBlock &) = default;
    auto operator=(const ProtectedByteBlock &) -> ProtectedByteBlock & = default;

public:
    /// Erase this block's encrypted envelope and restore the empty state.
    void secureErase() noexcept;
    /// Decrypt the block into sensitive storage.
    /// @return The authenticated plaintext, or an empty block for an empty protected block.
    /// @throws CryptologyError If the current application cannot authenticate or decrypt the envelope.
    [[nodiscard]] auto unprotect() const -> mem::ByteBlock;
    /// Invoke a callback with an authenticated temporary plaintext view.
    /// The temporary is securely erased before this method returns or propagates an exception.
    /// @param callback The callback to invoke exactly once.
    /// @throws err::ParameterError If the callback is empty.
    /// @throws CryptologyError If the current application cannot authenticate or decrypt the envelope.
    void withUnprotectedData(const UnprotectedDataFn &callback) const;

public: // tests
    /// Test whether this protected block contains no data.
    [[nodiscard]] auto isEmpty() const noexcept -> bool { return _byteLength.isZero(); }

public: // accessors
    /// Get the plaintext byte length without decrypting the block.
    [[nodiscard]] auto byteLength() const noexcept -> unit::ByteLength { return _byteLength; }

private:
    mem::ByteBlock _envelope;                               ///< Provider-owned encrypted and authenticated envelope.
    unit::ByteLength _byteLength{unit::ByteLength::zero()}; ///< Authenticated plaintext length metadata.
};

}
