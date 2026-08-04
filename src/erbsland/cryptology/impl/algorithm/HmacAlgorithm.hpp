// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../SecureEraseGuard.hpp"

#include "../../../err/LogicError.hpp"
#include "../../../err/ParameterError.hpp"
#include "../../../mem/ByteArray.hpp"
#include "../../../mem/ByteSpan.hpp"
#include "../../../text/Literals.hpp"

#include <cstddef>
#include <cstdint>
#include <utility>

namespace erbsland::cryptology::impl {

/// Implements reusable streaming HMAC as specified by RFC 2104 section 2.
/// The retained initial states are the hash state after processing `K0 xor ipad` and `K0 xor opad`. Precomputing these
/// states is equivalent to the RFC construction and permits `reset()` without retaining a separate normalized key.
/// Specification: https://www.rfc-editor.org/rfc/rfc2104.html#section-2
/// @tparam tHash The concrete streaming hash implementation.
/// @tparam tBlockSize The RFC 2104 hash compression-block size `B` in bytes.
/// @tparam tMaximumMessageLength The maximum message length after accounting for the preloaded inner-pad block.
/// @tested{HmacTest HashPrimitiveFullValidationTest PasswordPrimitiveTest}
template <typename tHash, std::size_t tBlockSize, uint64_t tMaximumMessageLength>
class HmacAlgorithm final {
private:
    using Digest = decltype(std::declval<tHash &>().digest());

public:
    /// Create keyed HMAC state.
    /// @param key The exact secret key bytes.
    explicit HmacAlgorithm(mem::ConstByteSpan key) { initialize(key); }

    /// Securely erase the retained keyed hash state.
    ~HmacAlgorithm() { secureErase(); }

    // defaults/deletions
    HmacAlgorithm(const HmacAlgorithm &) = delete;
    HmacAlgorithm(HmacAlgorithm &&) = delete;
    auto operator=(const HmacAlgorithm &) -> HmacAlgorithm & = delete;
    auto operator=(HmacAlgorithm &&) -> HmacAlgorithm & = delete;

public:
    /// Reset the message while retaining the same key.
    void reset() {
        // Restore H(K0 xor ipad || ...) to its state immediately before the RFC 2104 message text begins.
        _inner.secureErase();
        _inner = _initialInner;
        _digest.secureErase();
        _messageLength = 0U;
        _isFinalized = false;
    }
    /// Securely erase all keyed and message-dependent state.
    void secureErase() noexcept {
        // The precomputed hash states contain key-dependent material even though the normalized key is not retained.
        _initialInner.secureErase();
        _initialOuter.secureErase();
        _inner.secureErase();
        _digest.secureErase();
        _messageLength = 0U;
        _isFinalized = false;
    }
    /// Add exact message bytes.
    /// @param data The next bytes of the authenticated message.
    /// @throws err::LogicError If this state is already finalized.
    /// @throws err::ParameterError If the hash message-length limit would be exceeded.
    void update(mem::ConstByteSpan data) {
        using namespace text::literals;
        if (_isFinalized) {
            throw err::LogicError{"Cannot add HMAC data after finalization."_el};
        }
        const auto dataLength = static_cast<uint64_t>(data.size());
        if (dataLength > tMaximumMessageLength - _messageLength) {
            throw err::ParameterError{"The HMAC message is too long."_el, "data"_el};
        }
        // RFC 2104 calls this input `text`; it follows the already processed `K0 xor ipad` block.
        _inner.update(data);
        _messageLength += dataLength;
    }
    /// Finalize the message or return the cached complete authenticator.
    /// @return The full hash-sized HMAC value.
    [[nodiscard]] auto finalize() -> Digest {
        if (_isFinalized) {
            return _digest;
        }
        // RFC 2104 section 2: H((K0 xor opad) || H((K0 xor ipad) || text)).
        auto innerDigest = _inner.digest();
        // The inner digest and outer working state are key-derived intermediates and are erased on every exit path.
        [[maybe_unused]] const auto innerDigestErase = SecureEraseGuard{innerDigest};
        auto outer = _initialOuter;
        [[maybe_unused]] const auto outerErase = SecureEraseGuard{outer};
        outer.update(innerDigest.span());
        _digest = outer.digest();
        _isFinalized = true;
        return _digest;
    }

private:
    /// Normalize the key, create the two HMAC pads, and initialize retained hash states.
    void initialize(const mem::ConstByteSpan key) {
        // RFC 2104 section 2 defines K0 as a B-byte key: hash keys longer than B and zero-pad shorter keys.
        // Value-initialization provides the required zero padding for bytes not overwritten below.
        auto normalizedKey = mem::ByteArray<tBlockSize>{};
        // Every key-derived temporary below has a scope guard so exceptional exits erase it before unwinding.
        [[maybe_unused]] const auto normalizedKeyErase = SecureEraseGuard{normalizedKey};
        if (key.size() > tBlockSize) {
            auto keyHasher = tHash{};
            [[maybe_unused]] const auto keyHasherErase = SecureEraseGuard{keyHasher};
            keyHasher.update(key);
            auto keyDigest = keyHasher.digest();
            [[maybe_unused]] const auto keyDigestErase = SecureEraseGuard{keyDigest};
            normalizedKey.overwrite(keyDigest.span());
        } else {
            normalizedKey.overwrite(key);
        }

        // RFC 2104 section 2 defines ipad as B repetitions of 0x36 and opad as B repetitions of 0x5c.
        auto innerPad = mem::ByteArray<tBlockSize>{};
        [[maybe_unused]] const auto innerPadErase = SecureEraseGuard{innerPad};
        auto outerPad = mem::ByteArray<tBlockSize>{};
        [[maybe_unused]] const auto outerPadErase = SecureEraseGuard{outerPad};
        innerPad.fill(mem::Byte{0x36U});
        outerPad.fill(mem::Byte{0x5cU});
        // All operands have the compile-time size B, so the exact-size ByteArray operation cannot partially apply.
        innerPad ^= normalizedKey;
        outerPad ^= normalizedKey;

        // Pre-hash the two complete pad blocks. This is the RFC 2104 section 4 precomputation optimization.
        _initialInner.update(innerPad.span());
        _initialOuter.update(outerPad.span());
        _inner = _initialInner;
    }

private:
    tHash _initialInner;         ///< The keyed inner state used when resetting.
    tHash _initialOuter;         ///< The keyed outer state used when finalizing.
    tHash _inner;                ///< The current inner message state.
    Digest _digest{};            ///< The cached complete authenticator.
    uint64_t _messageLength{0U}; ///< The accepted message length in bytes.
    bool _isFinalized{false};    ///< Whether the cached authenticator is valid.
};

}
