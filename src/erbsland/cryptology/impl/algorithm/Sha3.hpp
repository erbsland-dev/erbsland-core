// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Keccak.hpp"

#include "../../../err/LogicError.hpp"
#include "../../../mem/ByteArray.hpp"
#include "../../../mem/ByteSpan.hpp"
#include "../../../mem/impl/SecureErase.hpp"
#include "../../../text/Literals.hpp"

#include <algorithm>
#include <span>

namespace erbsland::cryptology::impl {

// algorithms are never included in the public API, therefore using these namespaces never leaks.
using namespace erbsland::unit;
using namespace erbsland::mem;

/// A SHA-3 sponge with fixed rate and digest size.
/// @tparam tRateBytes The sponge rate in bytes.
/// @tparam tDigestBytes The fixed digest size in bytes.
/// @tested{Sha3ValidationTest HashFullValidationTest}
template <std::size_t tRateBytes, std::size_t tDigestBytes>
class Sha3 final {
public:
    /// Create a new instance for the algorithm and initialize the state.
    Sha3() { _state.fill(0); }

public:
    /// Reset the instance to allow processing another hash.
    void reset() {
        _state.fill(0);
        _bufferPosition = ByteIndex::zero();
        _hasDigest = false;
    }
    /// Securely erase all message-dependent state and reset this instance.
    void secureErase() noexcept {
        mem::impl::secureErase(std::as_writable_bytes(std::span{_state}));
        _buffer.secureErase();
        _digest.secureErase();
        _bufferPosition = ByteIndex::zero();
        _hasDigest = false;
        reset();
    }

    /// Update the hash with more data.
    /// @param data A block of data.
    /// @throws err::LogicError If data is added after the digest was calculated.
    void update(const ConstByteSpan data) {
        using namespace text::literals;
        if (_hasDigest) {
            throw err::LogicError("Adding more data, via `update()` after calling `digest()` is not allowed."_el);
        }
        auto dataPosition = std::size_t{};
        while (dataPosition < data.size()) {
            const auto copyLength = std::min(tRateBytes - _bufferPosition.toSizeT(), data.size() - dataPosition);
            _buffer.overwrite(_bufferPosition, data.subspan(dataPosition, copyLength));
            _bufferPosition.uncheckedAdvance(ByteLength::fromSizeT(copyLength));
            dataPosition += copyLength;
            if (_bufferPosition == _buffer.endIndex()) {
                absorbBlock();
            }
        }
    }

    /// Finalize the hash and return the digest.
    /// @return The digest.
    [[nodiscard]] auto digest() -> ByteArray<tDigestBytes> {
        if (_hasDigest) {
            return _digest;
        }
        finalize();
        return _digest;
    }

private:
    /// Finalize the hash calculation.
    /// - Adds the required padding to the current buffer.
    /// - Absorbs the last block and calculates the digest from the state.
    void finalize() {
        // clear the remaining part of the buffer with zeros.
        _buffer.fill(ByteRange{_bufferPosition, _buffer.length() - _bufferPosition.distanceFromZero()}, Byte{});
        // Apply the padding.
        _buffer.xorAt(_bufferPosition, Byte{0x06U});
        _buffer.xorAt(_buffer.endIndex().decremented(), Byte{0x80U});
        absorbBlock();
        // Every supported fixed digest fits into one rate block and consists of complete 64-bit lanes.
        auto digestIndex = ByteIndex::zero();
        for (const auto lane : std::span{_state}.first(tDigestBytes / sizeof(uint64_t))) {
            _digest.setIntegerOrThrow(digestIndex, lane);
            digestIndex.uncheckedAdvance(ByteLength{sizeof(uint64_t)});
        }
        _hasDigest = true;
    }

    /// Absorbs a completed block from the buffer.
    void absorbBlock() {
        // Keccak lanes use little-endian byte order. Reading complete lanes avoids rebuilding each word byte by byte.
        auto bufferIndex = ByteIndex::zero();
        for (auto &lane : std::span{_state}.first(tRateBytes / sizeof(uint64_t))) {
            lane ^= _buffer.template getInteger<uint64_t>(bufferIndex);
            bufferIndex.uncheckedAdvance(ByteLength{sizeof(uint64_t)});
        }
        keccakF1600Permutation(_state);
        _bufferPosition = ByteIndex::zero();
    }

private:
    KeccakF1600State _state{};         ///< The current state.
    ByteArray<tRateBytes> _buffer{};   ///< The rate block waiting to be absorbed.
    ByteIndex _bufferPosition{};       ///< The write-position in the buffer.
    ByteArray<tDigestBytes> _digest{}; ///< A cache for the final digest.
    bool _hasDigest = false;           ///< Flag that the digest was calculated and is ready to use.
};

/// The SHA3-256 hash algorithm with a 1088-bit rate and 256-bit output.
using Sha3_256 = Sha3<136, 32>;

/// The SHA3-384 hash algorithm with an 832-bit rate and 384-bit output.
using Sha3_384 = Sha3<104, 48>;

/// The SHA3-512 hash algorithm with a 576-bit rate and 512-bit output.
using Sha3_512 = Sha3<72, 64>;

}
