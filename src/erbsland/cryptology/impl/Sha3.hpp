// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Keccak.hpp"

#include "../../err/LogicError.hpp"

#include <span>
#include <stdexcept>

namespace erbsland::cryptology::impl {

/// The SHA3 implementation for the hash algorithm.
template <std::size_t tRateBytes, std::size_t tDigestBytes>
class Sha3 final {
public:
    /// Create a new instance for the algorithm and initialize the state.
    Sha3() { _state.fill(0); }

public:
    /// Reset the instance to allow processing another hash.
    void reset() {
        _state.fill(0);
        _bufferPosition = 0;
        _hasDigest = false;
    }

    /// Update the hash with more data.
    /// @param data A block of data.
    void update(const std::span<const std::byte> data) {
        if (_hasDigest) {
            throw err::LogicError("Adding more data, via `update()` after calling `digest()` is not allowed.");
        }
        for (std::byte dataByte : data) {
            _buffer[_bufferPosition] = static_cast<uint8_t>(dataByte);
            _bufferPosition += 1;
            if (_bufferPosition == tRateBytes) {
                absorbBlock();
            }
        }
    }

    /// Finalize the hash and return the digest.
    /// @return The digest.
    auto digest() -> std::array<std::byte, tDigestBytes> {
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
        for (std::size_t i = _bufferPosition; i < tRateBytes; ++i) {
            _buffer[i] = 0x00;
        }
        // Apply the padding.
        _buffer[_bufferPosition] ^= 0x06;
        _buffer[tRateBytes - 1] ^= 0x80;
        absorbBlock();
        std::size_t digestPosition = 0;
        while (digestPosition < tDigestBytes) {
            for (std::size_t i = 0; i < tRateBytes && digestPosition < tDigestBytes; ++i) {
                _digest[digestPosition] = std::byte{static_cast<uint8_t>((_state[i / 8] >> (8 * (i % 8))) & 0xff)};
                digestPosition += 1;
            }
            keccakF1600Permutation(_state);
        }
        _hasDigest = true;
    }

    /// Absorbs a completed block from the buffer.
    void absorbBlock() {
        for (std::size_t i = 0; i < tRateBytes; ++i) {
            _state[i / 8] ^= static_cast<uint64_t>(_buffer[i]) << (8 * (i % 8));
        }
        keccakF1600Permutation(_state);
        _bufferPosition = 0;
    }

private:
    KeccakF1600State _state{};                     ///< The current state.
    std::array<uint8_t, tRateBytes> _buffer{};     ///< The read buffer (uninitialized - will be overwritten)
    std::size_t _bufferPosition = 0;               ///< The write-position in the buffer.
    std::array<std::byte, tDigestBytes> _digest{}; ///< A cache for the final digest.
    bool _hasDigest = false;                       ///< Flag that the digest was calculated and is ready to use.
};

/// The SHA3-256 Hash Algorithm.
/// 1088-bit rate, 256-bit output
using Sha3_256 = Sha3<136, 32>;

/// The SHA3-384 Hash Algorithm.
/// 832-bit rate, 384-bit output
using Sha3_384 = Sha3<104, 48>;

/// The SHA3-512 Hash Algorithm.
/// 576-bit rate, 512-bit output
using Sha3_512 = Sha3<72, 64>;

}
