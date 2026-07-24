// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../err/LogicError.hpp"
#include "../../../err/ParameterError.hpp"
#include "../../../math/IntegerBitOperations.hpp"
#include "../../../mem/ByteArray.hpp"
#include "../../../mem/ByteBlockEditor.hpp"
#include "../../../mem/ByteSpan.hpp"
#include "../../../mem/impl/SecureErase.hpp"
#include "../../../unit/ByteIndex.hpp"
#include "../../../unit/ByteLength.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace erbsland::cryptology::impl {

// algorithms are never included in the public API, therefore using these namespaces never leaks.
using namespace erbsland::unit;
using namespace erbsland::mem;

/// The BLAKE2b hash used by Argon2.
///
/// BLAKE2b iterates a twelve-round compression function over 128-byte blocks. Its state consists of eight chaining
/// words, a 128-bit byte counter, and two final-block flags. The permutation schedule and mixing function below follow
/// RFC 7693 exactly; all message words and output words use little-endian byte order.
///
/// Source: https://www.rfc-editor.org/rfc/rfc7693.html
/// @tested{PasswordPrimitiveTest}
class Blake2b final {
public:
    /// Create an unkeyed BLAKE2b instance with a selected digest length.
    /// @param digestLength The digest length from 1 through 64 bytes.
    /// @throws err::ParameterError If `digestLength` is outside the supported range.
    explicit Blake2b(const ByteLength digestLength) : _digestLength{digestLength} {
        if (digestLength.isZero() || digestLength > ByteLength{64U}) {
            throw err::ParameterError{"BLAKE2b digest length must be between 1 and 64 bytes", "digestLength"};
        }
        _state = cInitializationVector;
        // RFC 7693 section 2.5 packs digest length into bits 0-7, key length into 8-15, fanout into 16-23, and
        // depth into 24-31. Therefore 0x01010000 means key length 0, fanout 1, depth 1; the digest length fills the
        // low byte for sequential, unkeyed hashing.
        _state[0] ^= 0x01010000U ^ static_cast<uint64_t>(digestLength.toRawValue());
    }

    /// Erase the chaining state and partial message block.
    ~Blake2b() { secureErase(); }
    Blake2b(const Blake2b &) = delete;
    Blake2b(Blake2b &&) = delete;
    auto operator=(const Blake2b &) -> Blake2b & = delete;
    auto operator=(Blake2b &&) -> Blake2b & = delete;

public:
    /// Securely erase all message-dependent state and reset this instance.
    void secureErase() noexcept {
        mem::impl::secureErase(std::as_writable_bytes(std::span{_state}));
        _buffer.secureErase();
        _counterLow = 0;
        _counterHigh = 0;
        _bufferLength = ByteLength::zero();
        _finalized = false;
        _state = cInitializationVector;
        _state[0] ^= 0x01010000U ^ static_cast<uint64_t>(_digestLength.toRawValue());
    }

    /// Hash one byte sequence with BLAKE2b in one call.
    /// Equals a call of `update` followed by `digest`.
    /// @param input The complete message.
    /// @return The digest in marked storage.
    [[nodiscard]] auto digest(const ConstByteSpan input) -> ByteBlockEditor {
        update(input);
        return digest();
    }
    /// Add exact message bytes.
    /// @param input The next message bytes.
    /// @throws err::LogicError If the digest was already finalized.
    void update(const ConstByteSpan input) {
        if (_finalized) {
            throw err::LogicError{"BLAKE2b input cannot be added after finalization"};
        }
        // Retain one full block until more input arrives. RFC 7693 section 3.3 marks the actual final block during
        // compression, so a message ending exactly at a block boundary must not compress that block early.
        auto position = ByteIndex::zero();
        while (position.isWithin(ByteLength::fromSizeT(input.size()))) {
            if (_bufferLength == _buffer.length()) {
                // The buffered block is no longer final. Count its 128 message bytes, compress it without f0, then
                // reuse the buffer for the following input.
                addCounter(_buffer.length());
                compress(false);
                _bufferLength = ByteLength::zero();
            }
            // Copy only enough input to complete the retained block; later iterations continue with the remainder.
            const auto count = std::min(
                _buffer.length() - _bufferLength, ByteLength::fromSizeT(input.size()) - position.distanceFromZero());
            _buffer.overwrite(ByteIndex::end(_bufferLength), input.subspan(position.toSizeT(), count.toSizeT()));
            _bufferLength += count;
            position += count;
        }
    }

    /// Finalize and return the selected number of digest bytes in marked storage.
    /// @return The configured 1-64 digest bytes.
    /// @throws err::LogicError If this instance was already finalized.
    [[nodiscard]] auto digest() -> ByteBlockEditor {
        if (_finalized) {
            throw err::LogicError{"BLAKE2b digest was already finalized"};
        }
        // RFC 7693's counter is the number of message bytes through this final block, excluding zero padding.
        addCounter(_bufferLength);
        // BLAKE2 permits a partial final block. The missing bytes are zero before the last-block flag is applied.
        _buffer.fill(ByteRange{ByteIndex::end(_bufferLength), _buffer.length() - _bufferLength}, Byte{});
        compress(true);
        // RFC 7693 section 3.3 serializes h[0..7] little-endian and truncates to the configured digest length.
        auto result = ByteBlockEditor{_digestLength};
        result.markAsSensitive();
        auto position = ByteIndex::zero();
        while ((position + ByteLength{sizeof(uint64_t)}).isWithin(_digestLength)) {
            result.setIntegerOrThrow(position, _state[position.toSizeT() / sizeof(uint64_t)]);
            position += ByteLength{sizeof(uint64_t)};
        }
        while (position.isWithin(_digestLength)) {
            result.set(
                position,
                Byte::fromCroppedUInt64(
                    _state[position.toSizeT() / sizeof(uint64_t)] >> ((position.toSizeT() % 8U) * 8U)));
            ++position;
        }
        _finalized = true;
        return result;
    }

private:
    /// Apply the BLAKE2b G mixing function from RFC 7693 section 3.1.
    static void mix(uint64_t &a, uint64_t &b, uint64_t &c, uint64_t &d, const uint64_t x, const uint64_t y) noexcept {
        // These 32, 24, 16, and 63-bit rotations are the BLAKE2b-specific constants in RFC 7693 section 3.1.
        a = a + b + x;
        d = math::rotateRight(d ^ a, 32);
        c = c + d;
        b = math::rotateRight(b ^ c, 24);
        a = a + b + y;
        d = math::rotateRight(d ^ a, 16);
        c = c + d;
        b = math::rotateRight(b ^ c, 63);
    }

    /// Add processed bytes to BLAKE2b's portable 128-bit counter.
    void addCounter(const ByteLength bytes) noexcept {
        const auto previous = _counterLow;
        _counterLow += bytes.toRawValue();
        if (_counterLow < previous) {
            ++_counterHigh;
        }
    }

    /// Compress the retained 128-byte message block.
    void compress(const bool isLast) noexcept {
        // RFC 7693 section 3.2 interprets the block as sixteen little-endian 64-bit message words.
        auto message = std::array<uint64_t, 16>{};
        auto work = std::array<uint64_t, 16>{};
        for (auto i = std::size_t{0}; i < message.size(); ++i) {
            message[i] = _buffer.getInteger<uint64_t>(ByteIndex::fromSizeT(i * sizeof(uint64_t)));
        }
        // Initialize v[0..7] from the chaining value and v[8..15] from the fixed initialization vector.
        for (auto index = std::size_t{}; index < _state.size(); ++index) {
            work[index] = _state[index];
            work[index + 8U] = cInitializationVector[index];
        }
        // Inject the low/high halves of byte counter t into v[12]/v[13].
        work[12] ^= _counterLow;
        work[13] ^= _counterHigh;
        if (isLast) {
            // RFC 7693 sets f0 to all ones for the last block by complementing v[14].
            work[14] = ~work[14];
        }
        // Each round applies four column G functions and four diagonal G functions. SIGMA selects the two message
        // words injected by each G call.
        for (auto round = std::size_t{0}; round < 12U; ++round) {
            const auto &s = cPermutation[round];
            mix(work[0], work[4], work[8], work[12], message[s[0]], message[s[1]]);
            mix(work[1], work[5], work[9], work[13], message[s[2]], message[s[3]]);
            mix(work[2], work[6], work[10], work[14], message[s[4]], message[s[5]]);
            mix(work[3], work[7], work[11], work[15], message[s[6]], message[s[7]]);
            mix(work[0], work[5], work[10], work[15], message[s[8]], message[s[9]]);
            mix(work[1], work[6], work[11], work[12], message[s[10]], message[s[11]]);
            mix(work[2], work[7], work[8], work[13], message[s[12]], message[s[13]]);
            mix(work[3], work[4], work[9], work[14], message[s[14]], message[s[15]]);
        }
        // Feed both halves of the work vector back into h, exactly h[i] := h[i] XOR v[i] XOR v[i+8].
        for (auto i = std::size_t{0}; i < _state.size(); ++i) {
            _state[i] ^= work[i] ^ work[i + 8U];
        }
        mem::impl::secureErase(std::as_writable_bytes(std::span{message}));
        mem::impl::secureErase(std::as_writable_bytes(std::span{work}));
    }

private:
    // clang-format off
    // RFC 7693 section 2.6 defines this initialization vector as the SHA-512 IV. FIPS 180-4 section 5.3.5 derives
    // those words from the fractional parts of the square roots of the first eight prime numbers.
    inline static constexpr auto cInitializationVector = std::array<uint64_t, 8>{
        0x6a09e667f3bcc908ULL, 0xbb67ae8584caa73bULL, 0x3c6ef372fe94f82bULL, 0xa54ff53a5f1d36f1ULL,
        0x510e527fade682d1ULL, 0x9b05688c2b3e6c1fULL, 0x1f83d9abfb41bd6bULL, 0x5be0cd19137e2179ULL};
    // RFC 7693 section 2.7 defines SIGMA as ten message-word permutations. BLAKE2b runs twelve rounds, so rounds
    // ten and eleven repeat rows zero and one as prescribed by section 3.2.
    inline static constexpr auto cPermutation = std::array<std::array<uint8_t, 16>, 12>{{
        {{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15}},
        {{14,10, 4, 8, 9,15,13, 6, 1,12, 0, 2,11, 7, 5, 3}},
        {{11, 8,12, 0, 5, 2,15,13,10,14, 3, 6, 7, 1, 9, 4}},
        {{ 7, 9, 3, 1,13,12,11,14, 2, 6, 5,10, 4, 0,15, 8}},
        {{ 9, 0, 5, 7, 2, 4,10,15,14, 1,11,12, 6, 8, 3,13}},
        {{ 2,12, 6,10, 0,11, 8, 3, 4,13, 7, 5,15,14, 1, 9}},
        {{12, 5, 1,15,14,13, 4,10, 0, 7, 6, 3, 9, 2, 8,11}},
        {{13,11, 7,14,12, 1, 3, 9, 5, 0,15, 4, 8, 6, 2,10}},
        {{ 6,15,14, 9,11, 3, 0, 8,12, 2,13, 7, 1, 4,10, 5}},
        {{10, 2, 8, 4, 7, 6, 1, 5,15,11, 9,14, 3,12,13, 0}},
        {{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15}},
        {{14,10, 4, 8, 9,15,13, 6, 1,12, 0, 2,11, 7, 5, 3}},
    }};
    // clang-format on

    std::array<uint64_t, 8> _state{}; ///< The chaining value h[0..7].
    ByteArray<128> _buffer{};         ///< The retained 128-byte message block.
    ByteLength _bufferLength{};       ///< The number of message bytes in `_buffer`.
    uint64_t _counterLow{};           ///< The low half of the processed-byte counter t.
    uint64_t _counterHigh{};          ///< The high half of the processed-byte counter t.
    ByteLength _digestLength{};       ///< The configured digest length in bytes.
    bool _finalized{};                ///< Whether the final block was compressed.
};

}
