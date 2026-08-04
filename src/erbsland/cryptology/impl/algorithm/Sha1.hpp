// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../err/LogicError.hpp"
#include "../../../math/IntegerBitOperations.hpp"
#include "../../../mem/ByteArray.hpp"
#include "../../../mem/ByteSpan.hpp"
#include "../../../mem/impl/SecureErase.hpp"
#include "../../../unit/ByteIndex.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace erbsland::cryptology::impl {

// algorithms are never included in the public API, therefore using these namespaces never leaks.
using namespace erbsland::unit;
using namespace erbsland::mem;

/// The legacy SHA-1 streaming hash implementation.
///
/// SHA-1 is retained exclusively for verification of legacy data. Practical collisions have been demonstrated,
/// therefore SHA-1 must not be used to protect new data.
///
/// Like MD5, SHA-1 is a Merkle-Damgård construction: the padded message is split into 512-bit blocks, and each block
/// updates a 160-bit chaining state. All words are read and written in big-endian byte order.
///
/// Source: https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.180-4.pdf (section 6.1)
/// @tested{HashValidationTest HashFullValidationTest}
class Sha1 final {
public:
    /// Create a new instance and initialize the state.
    Sha1() { reset(); }

public:
    /// Reset the state to the SHA-1 initialization vector.
    ///
    /// The five words H0 to H4 are the fixed starting values from FIPS 180-4, section 5.3.1. The first four are the
    /// same counting pattern as in MD5, extended by one word.
    void reset() {
        _state = {0x67452301U, 0xefcdab89U, 0x98badcfeU, 0x10325476U, 0xc3d2e1f0U};
        _bufferPosition = ByteIndex::zero();
        _bitLength = 0;
        _hasDigest = false;
    }
    /// Securely erase all message-dependent state and reset this instance.
    void secureErase() noexcept {
        mem::impl::secureErase(std::as_writable_bytes(std::span{_state}));
        _buffer.secureErase();
        _digest.secureErase();
        _bufferPosition = ByteIndex::zero();
        _bitLength = 0;
        _hasDigest = false;
        reset();
    }

    /// Add bytes to the current message.
    ///
    /// Bytes are collected in the buffer until a full 512-bit block is available, which is compressed immediately.
    /// Therefore, a message of any size can be hashed as a stream, and the result never depends on how the data is
    /// split across calls.
    ///
    /// @param data A block of data.
    /// @throws err::LogicError if data is added after the digest was calculated.
    void update(const ConstByteSpan data) {
        if (_hasDigest) {
            throw err::LogicError{"Adding data after finalizing a hash is not allowed."};
        }
        auto dataPosition = std::size_t{};
        while (dataPosition < data.size()) {
            const auto copyLength =
                std::min(_buffer.length().toSizeT() - _bufferPosition.toSizeT(), data.size() - dataPosition);
            _buffer.overwrite(_bufferPosition, data.subspan(dataPosition, copyLength));
            _bufferPosition.uncheckedAdvance(ByteLength::fromSizeT(copyLength));
            dataPosition += copyLength;
            _bitLength += static_cast<uint64_t>(copyLength) * 8U;
            if (_bufferPosition == _buffer.endIndex()) {
                processBlock(_buffer);
                _bufferPosition = ByteIndex::zero();
            }
        }
    }

    /// Finalize the message and return the cached digest.
    ///
    /// Pads the message, compresses the remaining block(s) and serializes the state. The result is cached, therefore
    /// the finalization runs only once and further calls are cheap.
    ///
    /// @return The digest of 20 bytes (160 bits).
    [[nodiscard]] auto digest() -> ByteArray<20> {
        if (_hasDigest) {
            return _digest;
        }
        // SHA-1 appends one marker bit, zeros, and the original 64-bit big-endian bit length. As the message is
        // always a whole number of bytes, the marker bit is the byte 0x80.
        _buffer.set(_bufferPosition, Byte{0x80U});
        _bufferPosition.uncheckedIncrement();
        // If the marker left no room for the eight length bytes, complete this block with zeros and compress it.
        // The length then goes into one additional, otherwise empty block.
        if (_bufferPosition > ByteIndex{56U}) {
            _buffer.fill(ByteRange{_bufferPosition, _buffer.length() - _bufferPosition.distanceFromZero()}, Byte{});
            processBlock(_buffer);
            _bufferPosition = ByteIndex::zero();
        }
        // Zero the gap up to the length field, write the bit length, and compress the final block.
        _buffer.fill(ByteRange{_bufferPosition, ByteLength{56U} - _bufferPosition.distanceFromZero()}, Byte{});
        _buffer.setIntegerOrThrow(ByteIndex{56U}, _bitLength, Endianness::Big);
        processBlock(_buffer);
        // The digest is the final state H0 to H4, with each word written as four big-endian bytes.
        auto digestIndex = ByteIndex::zero();
        for (const auto word : _state) {
            _digest.setIntegerOrThrow(digestIndex, word, Endianness::Big);
            digestIndex.uncheckedAdvance(ByteLength{sizeof(uint32_t)});
        }
        _hasDigest = true;
        return _digest;
    }

private:
    /// Select bits from `whenSet` or `whenClear` using `selector`.
    [[nodiscard]] static constexpr auto choice(
        const uint32_t selector, const uint32_t whenSet, const uint32_t whenClear) noexcept -> uint32_t {
        return (selector & whenSet) | (~selector & whenClear);
    }
    /// Calculate the parity of three words.
    [[nodiscard]] static constexpr auto parity(
        const uint32_t first, const uint32_t second, const uint32_t third) noexcept -> uint32_t {
        return first ^ second ^ third;
    }
    /// Select the bits present in at least two of three words.
    [[nodiscard]] static constexpr auto majority(
        const uint32_t first, const uint32_t second, const uint32_t third) noexcept -> uint32_t {
        return (first & second) | (first & third) | (second & third);
    }
    /// Apply one SHA-1 compression round.
    /// The caller rotates the state roles through the arguments. Only `b` and `e` receive new values.
    static constexpr void applyRound(
        const uint32_t a,
        uint32_t &b,
        const uint32_t function,
        uint32_t &e,
        const uint32_t word,
        const uint32_t constant) noexcept {
        e += math::rotateLeft(a, 5) + function + constant + word;
        b = math::rotateLeft(b, 30);
    }

    /// Expand and compress one 512-bit message block.
    ///
    /// @param block The 64 bytes of the block.
    void processBlock(const ByteArray<64> &block) {
        // The message schedule W. Its first sixteen words are the block itself, read as big-endian 32-bit words.
        auto words = std::array<uint32_t, 80>{};
        auto blockIndex = ByteIndex::zero();
        for (auto &word : std::span{words}.first<16U>()) {
            word = block.getInteger<uint32_t>(blockIndex, Endianness::Big);
            blockIndex.uncheckedAdvance(ByteLength{sizeof(uint32_t)});
        }
        // The remaining 64 words are derived from four earlier words. The one-bit rotation is the only change from
        // the withdrawn SHA-0, and it is what prevents differences from cancelling out over the schedule.
        for (std::size_t i = 16U; i < words.size(); ++i) {
            words[i] = math::rotateLeft(
                static_cast<uint32_t>(words[i - 3U] ^ words[i - 8U] ^ words[i - 14U] ^ words[i - 16U]), 1);
        }

        // The working copy of the chaining state, which is merged back after the last round.
        auto a = _state[0];
        auto b = _state[1];
        auto c = _state[2];
        auto d = _state[3];
        auto e = _state[4];
        // Five calls return the variables to their original roles. Each phase names its nonlinear function and
        // constant directly, avoiding a branch and four state assignments in every round.
        for (auto i = std::size_t{}; i < 20U; i += 5U) {
            applyRound(a, b, choice(b, c, d), e, words[i], 0x5a827999U);
            applyRound(e, a, choice(a, b, c), d, words[i + 1U], 0x5a827999U);
            applyRound(d, e, choice(e, a, b), c, words[i + 2U], 0x5a827999U);
            applyRound(c, d, choice(d, e, a), b, words[i + 3U], 0x5a827999U);
            applyRound(b, c, choice(c, d, e), a, words[i + 4U], 0x5a827999U);
        }
        for (auto i = std::size_t{20U}; i < 40U; i += 5U) {
            applyRound(a, b, parity(b, c, d), e, words[i], 0x6ed9eba1U);
            applyRound(e, a, parity(a, b, c), d, words[i + 1U], 0x6ed9eba1U);
            applyRound(d, e, parity(e, a, b), c, words[i + 2U], 0x6ed9eba1U);
            applyRound(c, d, parity(d, e, a), b, words[i + 3U], 0x6ed9eba1U);
            applyRound(b, c, parity(c, d, e), a, words[i + 4U], 0x6ed9eba1U);
        }
        for (auto i = std::size_t{40U}; i < 60U; i += 5U) {
            applyRound(a, b, majority(b, c, d), e, words[i], 0x8f1bbcdcU);
            applyRound(e, a, majority(a, b, c), d, words[i + 1U], 0x8f1bbcdcU);
            applyRound(d, e, majority(e, a, b), c, words[i + 2U], 0x8f1bbcdcU);
            applyRound(c, d, majority(d, e, a), b, words[i + 3U], 0x8f1bbcdcU);
            applyRound(b, c, majority(c, d, e), a, words[i + 4U], 0x8f1bbcdcU);
        }
        for (auto i = std::size_t{60U}; i < words.size(); i += 5U) {
            applyRound(a, b, parity(b, c, d), e, words[i], 0xca62c1d6U);
            applyRound(e, a, parity(a, b, c), d, words[i + 1U], 0xca62c1d6U);
            applyRound(d, e, parity(e, a, b), c, words[i + 2U], 0xca62c1d6U);
            applyRound(c, d, parity(d, e, a), b, words[i + 3U], 0xca62c1d6U);
            applyRound(b, c, parity(c, d, e), a, words[i + 4U], 0xca62c1d6U);
        }
        // Feed the working state back into the chaining state. This addition is what makes the compression function
        // one-way; the wrap-around of the unsigned arithmetic is part of the algorithm.
        _state[0] += a;
        _state[1] += b;
        _state[2] += c;
        _state[3] += d;
        _state[4] += e;
    }

private:
    std::array<uint32_t, 5> _state{}; ///< The chaining state H0 to H4; after the last block it is the digest.
    ByteArray<64> _buffer{};          ///< Collects bytes until a full 512-bit block can be compressed.
    ByteIndex _bufferPosition{};      ///< The write-position in the buffer.
    uint64_t _bitLength{0};           ///< The length of the whole message in bits, required by the padding.
    ByteArray<20> _digest{};          ///< A cache for the final digest.
    bool _hasDigest{false};           ///< Flag that the digest was calculated and is ready to use.
};

}
