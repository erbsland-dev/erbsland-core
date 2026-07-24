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

/// The legacy MD5 streaming hash implementation.
///
/// MD5 is retained exclusively for verification of legacy data. Collisions can be produced within seconds on
/// ordinary hardware, therefore MD5 must not be used to protect new data.
///
/// MD5 is a Merkle-Damgård construction: the message is padded to a multiple of 512 bits, then each 512-bit block
/// updates a 128-bit chaining state. Unlike the SHA family, MD5 reads and writes all words in *little-endian* byte
/// order.
///
/// Source: https://www.rfc-editor.org/rfc/rfc1321
/// @tested{HashValidationTest}
class Md5 final {
public:
    /// Create a new instance and initialize the state.
    Md5() { reset(); }

public:
    /// Reset the state to the MD5 initialization vector.
    ///
    /// The four words A, B, C and D are the fixed starting values from RFC 1321, section 3.3. Written as
    /// little-endian bytes, they form the counting pattern 01 23 45 67 89 ab cd ef fe dc ba 98 76 54 32 10.
    void reset() {
        _state = {0x67452301U, 0xefcdab89U, 0x98badcfeU, 0x10325476U};
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
    /// @return The digest of 16 bytes (128 bits).
    [[nodiscard]] auto digest() -> ByteArray<16> {
        if (_hasDigest) {
            return _digest;
        }
        // MD5 appends one marker bit, zeros, and the original 64-bit little-endian bit length (RFC 1321, section
        // 3.1). As the message is always a whole number of bytes, the marker bit is the byte 0x80.
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
        _buffer.setIntegerOrThrow(ByteIndex{56U}, _bitLength);
        processBlock(_buffer);
        // The digest is the final state A, B, C, D, with each word written as four little-endian bytes.
        auto digestIndex = ByteIndex::zero();
        for (const auto word : _state) {
            _digest.setIntegerOrThrow(digestIndex, word);
            digestIndex.uncheckedAdvance(ByteLength{sizeof(uint32_t)});
        }
        _hasDigest = true;
        return _digest;
    }

private:
    /// Apply one MD5 compression operation.
    /// The caller rotates the state roles through the arguments. Only `a` receives a new value.
    static constexpr void applyRound(
        uint32_t &a,
        const uint32_t b,
        const uint32_t function,
        const uint32_t word,
        const uint32_t constant,
        const int shift) noexcept {
        a = b + math::rotateLeft(static_cast<uint32_t>(a + function + constant + word), shift);
    }

    /// Decode and compress one 512-bit message block.
    ///
    /// @param block The 64 bytes of the block.
    void processBlock(const ByteArray<64> &block) {
        // clang-format off
        // The left-rotation amount of each operation (RFC 1321, section 3.4). Each round of sixteen operations
        // cycles through the same four amounts.
        static constexpr auto shifts = std::array<uint8_t, 64>{
             7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,
             5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,
             4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,
             6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21,
        };
        // clang-format on

        // clang-format off
        // The additive constant of each operation, defined as K[i] = floor(2^32 * abs(sin(i + 1))) with the angle in
        // radians (RFC 1321, section 3.4). Deriving them from the sine function makes them "nothing up my sleeve"
        // numbers: values that are visibly not chosen to hide a weakness.
        static constexpr auto constants = std::array<uint32_t, 64>{
            0xd76aa478U, 0xe8c7b756U, 0x242070dbU, 0xc1bdceeeU, 0xf57c0fafU, 0x4787c62aU, 0xa8304613U, 0xfd469501U,
            0x698098d8U, 0x8b44f7afU, 0xffff5bb1U, 0x895cd7beU, 0x6b901122U, 0xfd987193U, 0xa679438eU, 0x49b40821U,
            0xf61e2562U, 0xc040b340U, 0x265e5a51U, 0xe9b6c7aaU, 0xd62f105dU, 0x02441453U, 0xd8a1e681U, 0xe7d3fbc8U,
            0x21e1cde6U, 0xc33707d6U, 0xf4d50d87U, 0x455a14edU, 0xa9e3e905U, 0xfcefa3f8U, 0x676f02d9U, 0x8d2a4c8aU,
            0xfffa3942U, 0x8771f681U, 0x6d9d6122U, 0xfde5380cU, 0xa4beea44U, 0x4bdecfa9U, 0xf6bb4b60U, 0xbebfbc70U,
            0x289b7ec6U, 0xeaa127faU, 0xd4ef3085U, 0x04881d05U, 0xd9d4d039U, 0xe6db99e5U, 0x1fa27cf8U, 0xc4ac5665U,
            0xf4292244U, 0x432aff97U, 0xab9423a7U, 0xfc93a039U, 0x655b59c3U, 0x8f0ccc92U, 0xffeff47dU, 0x85845dd1U,
            0x6fa87e4fU, 0xfe2ce6e0U, 0xa3014314U, 0x4e0811a1U, 0xf7537e82U, 0xbd3af235U, 0x2ad7d2bbU, 0xeb86d391U,
        };
        // clang-format on

        // Split the block into sixteen 32-bit words in little-endian order. MD5 has no message schedule: all four
        // rounds re-use these same sixteen words, each round in a different order.
        auto words = std::array<uint32_t, 16>{};
        auto blockIndex = ByteIndex::zero();
        for (auto &word : words) {
            word = block.getInteger<uint32_t>(blockIndex);
            blockIndex.uncheckedAdvance(ByteLength{sizeof(uint32_t)});
        }
        // The working copy of the chaining state, which is merged back after the last operation.
        auto a = _state[0];
        auto b = _state[1];
        auto c = _state[2];
        auto d = _state[3];
        // Four calls return the variables to their original roles. Splitting the four phases names the nonlinear
        // functions directly and avoids a branch and three state assignments in every operation.
        for (auto i = std::size_t{}; i < 16U; i += 4U) {
            applyRound(a, b, (b & c) | (~b & d), words[i], constants[i], shifts[i]);
            applyRound(d, a, (a & b) | (~a & c), words[i + 1U], constants[i + 1U], shifts[i + 1U]);
            applyRound(c, d, (d & a) | (~d & b), words[i + 2U], constants[i + 2U], shifts[i + 2U]);
            applyRound(b, c, (c & d) | (~c & a), words[i + 3U], constants[i + 3U], shifts[i + 3U]);
        }
        for (auto i = std::size_t{16U}; i < 32U; i += 4U) {
            applyRound(a, b, (d & b) | (~d & c), words[(5U * i + 1U) % 16U], constants[i], shifts[i]);
            applyRound(d, a, (c & a) | (~c & b), words[(5U * (i + 1U) + 1U) % 16U], constants[i + 1U], shifts[i + 1U]);
            applyRound(c, d, (b & d) | (~b & a), words[(5U * (i + 2U) + 1U) % 16U], constants[i + 2U], shifts[i + 2U]);
            applyRound(b, c, (a & c) | (~a & d), words[(5U * (i + 3U) + 1U) % 16U], constants[i + 3U], shifts[i + 3U]);
        }
        for (auto i = std::size_t{32U}; i < 48U; i += 4U) {
            applyRound(a, b, b ^ c ^ d, words[(3U * i + 5U) % 16U], constants[i], shifts[i]);
            applyRound(d, a, a ^ b ^ c, words[(3U * (i + 1U) + 5U) % 16U], constants[i + 1U], shifts[i + 1U]);
            applyRound(c, d, d ^ a ^ b, words[(3U * (i + 2U) + 5U) % 16U], constants[i + 2U], shifts[i + 2U]);
            applyRound(b, c, c ^ d ^ a, words[(3U * (i + 3U) + 5U) % 16U], constants[i + 3U], shifts[i + 3U]);
        }
        for (auto i = std::size_t{48U}; i < 64U; i += 4U) {
            applyRound(a, b, c ^ (b | ~d), words[(7U * i) % 16U], constants[i], shifts[i]);
            applyRound(d, a, b ^ (a | ~c), words[(7U * (i + 1U)) % 16U], constants[i + 1U], shifts[i + 1U]);
            applyRound(c, d, a ^ (d | ~b), words[(7U * (i + 2U)) % 16U], constants[i + 2U], shifts[i + 2U]);
            applyRound(b, c, d ^ (c | ~a), words[(7U * (i + 3U)) % 16U], constants[i + 3U], shifts[i + 3U]);
        }
        // Feed the working state back into the chaining state. This addition is what makes the compression function
        // one-way; the wrap-around of the unsigned arithmetic is part of the algorithm.
        _state[0] += a;
        _state[1] += b;
        _state[2] += c;
        _state[3] += d;
    }

private:
    std::array<uint32_t, 4> _state{}; ///< The chaining state A, B, C, D; after the last block it is the digest.
    ByteArray<64> _buffer{};          ///< Collects bytes until a full 512-bit block can be compressed.
    ByteIndex _bufferPosition{};      ///< The write-position in the buffer.
    uint64_t _bitLength{0};           ///< The length of the whole message in bits, required by the padding.
    ByteArray<16> _digest{};          ///< A cache for the final digest.
    bool _hasDigest{false};           ///< Flag that the digest was calculated and is ready to use.
};

}
