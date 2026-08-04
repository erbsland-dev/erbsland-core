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

/// The SHA-256 member of the SHA-2 family.
///
/// SHA-256 is a Merkle-Damgård construction: the padded message is split into 512-bit blocks, and each block updates
/// a 256-bit chaining state of eight 32-bit words. All words are read and written in big-endian byte order.
///
/// Compared to SHA-1, the message schedule and the round function mix far more aggressively, which is why the
/// shortcut attacks that broke SHA-1 do not carry over.
///
/// Source: https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.180-4.pdf (section 6.2)
/// @tested{HashValidationTest HashFullValidationTest}
class Sha2_256 final {
public:
    /// Create a new instance and initialize the state.
    Sha2_256() { reset(); }

public:
    /// Reset the state to the SHA-256 initialization vector.
    ///
    /// The eight words H0 to H7 are the first 32 bits of the fractional parts of the square roots of the first eight
    /// prime numbers (2, 3, 5, 7, 11, 13, 17, 19). See FIPS 180-4, section 5.3.3.
    void reset() {
        _state = {
            0x6a09e667U, 0xbb67ae85U, 0x3c6ef372U, 0xa54ff53aU, 0x510e527fU, 0x9b05688cU, 0x1f83d9abU, 0x5be0cd19U};
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
    /// @return The digest of 32 bytes (256 bits).
    [[nodiscard]] auto digest() -> ByteArray<32> {
        if (_hasDigest) {
            return _digest;
        }
        // SHA-256 appends one marker bit, zeros, and the original 64-bit big-endian bit length. As the message is
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
        // The digest is the final state H0 to H7, with each word written as four big-endian bytes.
        auto digestIndex = ByteIndex::zero();
        for (const auto word : _state) {
            _digest.setIntegerOrThrow(digestIndex, word, Endianness::Big);
            digestIndex.uncheckedAdvance(ByteLength{sizeof(uint32_t)});
        }
        _hasDigest = true;
        return _digest;
    }

private:
    /// Apply one SHA-256 compression round.
    /// The caller rotates the state roles through the arguments. Only `d` and `h` receive new values.
    static constexpr void applyRound(
        uint32_t &a,
        const uint32_t b,
        const uint32_t c,
        uint32_t &d,
        const uint32_t e,
        const uint32_t f,
        const uint32_t g,
        uint32_t &h,
        const uint32_t word,
        const uint32_t constant) noexcept {
        const auto sum1 = math::rotateRight(e, 6) ^ math::rotateRight(e, 11) ^ math::rotateRight(e, 25);
        const auto choice = (e & f) ^ (~e & g);
        const auto temporary1 = static_cast<uint32_t>(h + sum1 + choice + constant + word);
        const auto sum0 = math::rotateRight(a, 2) ^ math::rotateRight(a, 13) ^ math::rotateRight(a, 22);
        const auto majority = (a & b) ^ (a & c) ^ (b & c);
        const auto temporary2 = static_cast<uint32_t>(sum0 + majority);
        d += temporary1;
        h = static_cast<uint32_t>(temporary1 + temporary2);
    }

    /// Expand and compress one 512-bit message block.
    ///
    /// @param block The 64 bytes of the block.
    void processBlock(const ByteArray<64> &block) {
        // clang-format off
        // The additive constant of each round: the first 32 bits of the fractional parts of the cube roots of the
        // first sixty-four prime numbers (FIPS 180-4, section 4.2.2). Deriving them from cube roots makes them
        // "nothing up my sleeve" numbers: values that are visibly not chosen to hide a weakness.
        static constexpr auto constants = std::array<uint32_t, 64>{
            0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U, 0x3956c25bU, 0x59f111f1U, 0x923f82a4U, 0xab1c5ed5U,
            0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U, 0x72be5d74U, 0x80deb1feU, 0x9bdc06a7U, 0xc19bf174U,
            0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU, 0x2de92c6fU, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU,
            0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U, 0xc6e00bf3U, 0xd5a79147U, 0x06ca6351U, 0x14292967U,
            0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU, 0x53380d13U, 0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U,
            0xa2bfe8a1U, 0xa81a664bU, 0xc24b8b70U, 0xc76c51a3U, 0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U,
            0x19a4c116U, 0x1e376c08U, 0x2748774cU, 0x34b0bcb5U, 0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU, 0x682e6ff3U,
            0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U, 0x90befffaU, 0xa4506cebU, 0xbef9a3f7U, 0xc67178f2U,
        };
        // clang-format on

        // The message schedule W. Its first sixteen words are the block itself, read as big-endian 32-bit words.
        auto words = std::array<uint32_t, 64>{};
        auto blockIndex = ByteIndex::zero();
        for (auto &word : std::span{words}.first<16U>()) {
            word = block.getInteger<uint32_t>(blockIndex, Endianness::Big);
            blockIndex.uncheckedAdvance(ByteLength{sizeof(uint32_t)});
        }
        // Expand the sixteen input words into the sixty-four-word message schedule. `s0` and `s1` are the lower-case
        // sigma functions of FIPS 180-4: two rotations and one shift each. The shift makes them non-invertible,
        // which spreads the influence of every input bit over the whole schedule.
        for (std::size_t i = 16U; i < words.size(); ++i) {
            const auto s0 =
                math::rotateRight(words[i - 15U], 7) ^ math::rotateRight(words[i - 15U], 18) ^ (words[i - 15U] >> 3U);
            const auto s1 =
                math::rotateRight(words[i - 2U], 17) ^ math::rotateRight(words[i - 2U], 19) ^ (words[i - 2U] >> 10U);
            words[i] = static_cast<uint32_t>(words[i - 16U] + s0 + words[i - 7U] + s1);
        }
        // The working copy of the chaining state, which is merged back after the last round.
        auto a = _state[0];
        auto b = _state[1];
        auto c = _state[2];
        auto d = _state[3];
        auto e = _state[4];
        auto f = _state[5];
        auto g = _state[6];
        auto h = _state[7];
        // Eight calls return the variables to their original roles. Rotating the argument roles expresses the state
        // shift directly and avoids six assignments in every round.
        for (auto i = std::size_t{}; i < words.size(); i += 8U) {
            applyRound(a, b, c, d, e, f, g, h, words[i], constants[i]);
            applyRound(h, a, b, c, d, e, f, g, words[i + 1U], constants[i + 1U]);
            applyRound(g, h, a, b, c, d, e, f, words[i + 2U], constants[i + 2U]);
            applyRound(f, g, h, a, b, c, d, e, words[i + 3U], constants[i + 3U]);
            applyRound(e, f, g, h, a, b, c, d, words[i + 4U], constants[i + 4U]);
            applyRound(d, e, f, g, h, a, b, c, words[i + 5U], constants[i + 5U]);
            applyRound(c, d, e, f, g, h, a, b, words[i + 6U], constants[i + 6U]);
            applyRound(b, c, d, e, f, g, h, a, words[i + 7U], constants[i + 7U]);
        }
        // Feed the working state back into the chaining state. This addition is what makes the compression function
        // one-way; the wrap-around of the unsigned arithmetic is part of the algorithm.
        _state[0] += a;
        _state[1] += b;
        _state[2] += c;
        _state[3] += d;
        _state[4] += e;
        _state[5] += f;
        _state[6] += g;
        _state[7] += h;
    }

private:
    std::array<uint32_t, 8> _state{}; ///< The chaining state H0 to H7; after the last block it is the digest.
    ByteArray<64> _buffer{};          ///< Collects bytes until a full 512-bit block can be compressed.
    ByteIndex _bufferPosition{};      ///< The write-position in the buffer.
    uint64_t _bitLength{0};           ///< The length of the whole message in bits, required by the padding.
    ByteArray<32> _digest{};          ///< A cache for the final digest.
    bool _hasDigest{false};           ///< Flag that the digest was calculated and is ready to use.
};

/// Shared mechanics for the 64-bit SHA-384 and SHA-512 variants.
///
/// Both variants use the identical compression function and differ only in their initialization vector and in the
/// number of state words that are written to the digest. The structure mirrors SHA-256, but with 64-bit words:
/// 1024-bit blocks, eighty rounds, and different rotation amounts.
///
/// The 128-bit message length is stored as two portable 64-bit words, as there is no portable 128-bit integer type.
///
/// Source: https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.180-4.pdf (sections 6.4 and 6.5)
/// @tparam tDigestBytes The size of the digest in bytes; 48 selects SHA-384, 64 selects SHA-512.
/// @tested{HashValidationTest HashFullValidationTest}
template <std::size_t tDigestBytes>
class Sha2_64 final {
public:
    /// Create a new instance and initialize the state.
    Sha2_64() { reset(); }

public:
    /// Reset to the SHA-384 or SHA-512 initialization vector.
    ///
    /// SHA-512 starts from the first 64 bits of the fractional parts of the square roots of the first eight prime
    /// numbers (2 to 19). SHA-384 uses the ninth to sixteenth primes (23 to 53) instead, which is what makes it a
    /// distinct hash rather than a plain truncation of SHA-512. See FIPS 180-4, sections 5.3.4 and 5.3.5.
    void reset() {
        if constexpr (tDigestBytes == 48U) {
            _state = {
                0xcbbb9d5dc1059ed8ULL,
                0x629a292a367cd507ULL,
                0x9159015a3070dd17ULL,
                0x152fecd8f70e5939ULL,
                0x67332667ffc00b31ULL,
                0x8eb44a8768581511ULL,
                0xdb0c2e0d64f98fa7ULL,
                0x47b5481dbefa4fa4ULL};
        } else {
            _state = {
                0x6a09e667f3bcc908ULL,
                0xbb67ae8584caa73bULL,
                0x3c6ef372fe94f82bULL,
                0xa54ff53a5f1d36f1ULL,
                0x510e527fade682d1ULL,
                0x9b05688c2b3e6c1fULL,
                0x1f83d9abfb41bd6bULL,
                0x5be0cd19137e2179ULL};
        }
        _bufferPosition = ByteIndex::zero();
        _bitLengthHigh = 0;
        _bitLengthLow = 0;
        _hasDigest = false;
    }
    /// Securely erase all message-dependent state and reset this instance.
    void secureErase() noexcept {
        mem::impl::secureErase(std::as_writable_bytes(std::span{_state}));
        _buffer.secureErase();
        _digest.secureErase();
        _bufferPosition = ByteIndex::zero();
        _bitLengthHigh = 0;
        _bitLengthLow = 0;
        _hasDigest = false;
        reset();
    }

    /// Add bytes and update the portable high/low 128-bit message length.
    ///
    /// Bytes are collected in the buffer until a full 1024-bit block is available, which is compressed immediately.
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
            // Add eight bits to the 128-bit counter. If the low word wrapped around, carry into the high word.
            const auto previous = _bitLengthLow;
            _bitLengthLow += static_cast<uint64_t>(copyLength) * 8U;
            if (_bitLengthLow < previous) {
                ++_bitLengthHigh;
            }
            if (_bufferPosition == _buffer.endIndex()) {
                processBlock(_buffer);
                _bufferPosition = ByteIndex::zero();
            }
        }
    }

    /// Finalize the message and return the truncated or complete cached digest.
    ///
    /// Pads the message, compresses the remaining block(s) and serializes the state. The result is cached, therefore
    /// the finalization runs only once and further calls are cheap.
    ///
    /// @return The digest of `tDigestBytes` bytes.
    [[nodiscard]] auto digest() -> ByteArray<tDigestBytes> {
        if (_hasDigest) {
            return _digest;
        }
        // The padding follows the same scheme as SHA-256, but the length field is 128 bits wide. Therefore, the
        // message data of the last block must end before byte 112 of the 128-byte block.
        _buffer.set(_bufferPosition, Byte{0x80U});
        _bufferPosition.uncheckedIncrement();
        // If the marker left no room for the sixteen length bytes, complete this block with zeros and compress it.
        // The length then goes into one additional, otherwise empty block.
        if (_bufferPosition > ByteIndex{112U}) {
            _buffer.fill(ByteRange{_bufferPosition, _buffer.length() - _bufferPosition.distanceFromZero()}, Byte{});
            processBlock(_buffer);
            _bufferPosition = ByteIndex::zero();
        }
        // Zero the gap, then write the paired high/low bit length in big-endian order and compress the final block.
        _buffer.fill(ByteRange{_bufferPosition, ByteLength{112U} - _bufferPosition.distanceFromZero()}, Byte{});
        _buffer.setIntegerOrThrow(ByteIndex{112U}, _bitLengthHigh, Endianness::Big);
        _buffer.setIntegerOrThrow(ByteIndex{120U}, _bitLengthLow, Endianness::Big);
        processBlock(_buffer);
        // The digest is the leading part of the final state, with each word written as eight big-endian bytes.
        // SHA-384 keeps the first six words, SHA-512 all eight.
        auto digestIndex = ByteIndex::zero();
        for (const auto word : std::span{_state}.first(tDigestBytes / sizeof(uint64_t))) {
            _digest.setIntegerOrThrow(digestIndex, word, Endianness::Big);
            digestIndex.uncheckedAdvance(ByteLength{sizeof(uint64_t)});
        }
        _hasDigest = true;
        return _digest;
    }

private:
    /// Apply one SHA-384 or SHA-512 compression round.
    /// The caller rotates the state roles through the arguments. Only `d` and `h` receive new values.
    static constexpr void applyRound(
        uint64_t &a,
        const uint64_t b,
        const uint64_t c,
        uint64_t &d,
        const uint64_t e,
        const uint64_t f,
        const uint64_t g,
        uint64_t &h,
        const uint64_t word,
        const uint64_t constant) noexcept {
        const auto sum1 = math::rotateRight(e, 14) ^ math::rotateRight(e, 18) ^ math::rotateRight(e, 41);
        const auto choice = (e & f) ^ (~e & g);
        const auto temporary1 = h + sum1 + choice + constant + word;
        const auto sum0 = math::rotateRight(a, 28) ^ math::rotateRight(a, 34) ^ math::rotateRight(a, 39);
        const auto majority = (a & b) ^ (a & c) ^ (b & c);
        const auto temporary2 = sum0 + majority;
        d += temporary1;
        h = temporary1 + temporary2;
    }

    /// Expand and compress one 1024-bit message block.
    ///
    /// @param block The 128 bytes of the block.
    void processBlock(const ByteArray<128> &block) {
        // clang-format off
        // The additive constant of each round: the first 64 bits of the fractional parts of the cube roots of the
        // first eighty prime numbers (FIPS 180-4, section 4.2.3). Deriving them from cube roots makes them
        // "nothing up my sleeve" numbers: values that are visibly not chosen to hide a weakness.
        static constexpr auto constants = std::array<uint64_t, 80>{
            0x428a2f98d728ae22ULL, 0x7137449123ef65cdULL, 0xb5c0fbcfec4d3b2fULL, 0xe9b5dba58189dbbcULL,
            0x3956c25bf348b538ULL, 0x59f111f1b605d019ULL, 0x923f82a4af194f9bULL, 0xab1c5ed5da6d8118ULL,
            0xd807aa98a3030242ULL, 0x12835b0145706fbeULL, 0x243185be4ee4b28cULL, 0x550c7dc3d5ffb4e2ULL,
            0x72be5d74f27b896fULL, 0x80deb1fe3b1696b1ULL, 0x9bdc06a725c71235ULL, 0xc19bf174cf692694ULL,
            0xe49b69c19ef14ad2ULL, 0xefbe4786384f25e3ULL, 0x0fc19dc68b8cd5b5ULL, 0x240ca1cc77ac9c65ULL,
            0x2de92c6f592b0275ULL, 0x4a7484aa6ea6e483ULL, 0x5cb0a9dcbd41fbd4ULL, 0x76f988da831153b5ULL,
            0x983e5152ee66dfabULL, 0xa831c66d2db43210ULL, 0xb00327c898fb213fULL, 0xbf597fc7beef0ee4ULL,
            0xc6e00bf33da88fc2ULL, 0xd5a79147930aa725ULL, 0x06ca6351e003826fULL, 0x142929670a0e6e70ULL,
            0x27b70a8546d22ffcULL, 0x2e1b21385c26c926ULL, 0x4d2c6dfc5ac42aedULL, 0x53380d139d95b3dfULL,
            0x650a73548baf63deULL, 0x766a0abb3c77b2a8ULL, 0x81c2c92e47edaee6ULL, 0x92722c851482353bULL,
            0xa2bfe8a14cf10364ULL, 0xa81a664bbc423001ULL, 0xc24b8b70d0f89791ULL, 0xc76c51a30654be30ULL,
            0xd192e819d6ef5218ULL, 0xd69906245565a910ULL, 0xf40e35855771202aULL, 0x106aa07032bbd1b8ULL,
            0x19a4c116b8d2d0c8ULL, 0x1e376c085141ab53ULL, 0x2748774cdf8eeb99ULL, 0x34b0bcb5e19b48a8ULL,
            0x391c0cb3c5c95a63ULL, 0x4ed8aa4ae3418acbULL, 0x5b9cca4f7763e373ULL, 0x682e6ff3d6b2b8a3ULL,
            0x748f82ee5defb2fcULL, 0x78a5636f43172f60ULL, 0x84c87814a1f0ab72ULL, 0x8cc702081a6439ecULL,
            0x90befffa23631e28ULL, 0xa4506cebde82bde9ULL, 0xbef9a3f7b2c67915ULL, 0xc67178f2e372532bULL,
            0xca273eceea26619cULL, 0xd186b8c721c0c207ULL, 0xeada7dd6cde0eb1eULL, 0xf57d4f7fee6ed178ULL,
            0x06f067aa72176fbaULL, 0x0a637dc5a2c898a6ULL, 0x113f9804bef90daeULL, 0x1b710b35131c471bULL,
            0x28db77f523047d84ULL, 0x32caab7b40c72493ULL, 0x3c9ebe0a15c9bebcULL, 0x431d67c49c100d4cULL,
            0x4cc5d4becb3e42b6ULL, 0x597f299cfc657e2aULL, 0x5fcb6fab3ad6faecULL, 0x6c44198c4a475817ULL,
        };
        // clang-format on

        // The message schedule W. Its first sixteen words are the block itself, read as big-endian 64-bit words.
        auto words = std::array<uint64_t, 80>{};
        auto blockIndex = ByteIndex::zero();
        for (auto &word : std::span{words}.first<16U>()) {
            word = block.getInteger<uint64_t>(blockIndex, Endianness::Big);
            blockIndex.uncheckedAdvance(ByteLength{sizeof(uint64_t)});
        }
        // Expand the sixteen input words into the eighty-word message schedule. `s0` and `s1` are the lower-case
        // sigma functions; they use the same shape as SHA-256, with the rotation amounts of the 64-bit variants.
        for (std::size_t i = 16U; i < words.size(); ++i) {
            const auto s0 =
                math::rotateRight(words[i - 15U], 1) ^ math::rotateRight(words[i - 15U], 8) ^ (words[i - 15U] >> 7U);
            const auto s1 =
                math::rotateRight(words[i - 2U], 19) ^ math::rotateRight(words[i - 2U], 61) ^ (words[i - 2U] >> 6U);
            words[i] = words[i - 16U] + s0 + words[i - 7U] + s1;
        }
        // The working copy of the chaining state, which is merged back after the last round.
        auto a = _state[0];
        auto b = _state[1];
        auto c = _state[2];
        auto d = _state[3];
        auto e = _state[4];
        auto f = _state[5];
        auto g = _state[6];
        auto h = _state[7];
        // Eight calls return the variables to their original roles. Rotating the argument roles expresses the state
        // shift directly and avoids six assignments in every round.
        for (auto i = std::size_t{}; i < words.size(); i += 8U) {
            applyRound(a, b, c, d, e, f, g, h, words[i], constants[i]);
            applyRound(h, a, b, c, d, e, f, g, words[i + 1U], constants[i + 1U]);
            applyRound(g, h, a, b, c, d, e, f, words[i + 2U], constants[i + 2U]);
            applyRound(f, g, h, a, b, c, d, e, words[i + 3U], constants[i + 3U]);
            applyRound(e, f, g, h, a, b, c, d, words[i + 4U], constants[i + 4U]);
            applyRound(d, e, f, g, h, a, b, c, words[i + 5U], constants[i + 5U]);
            applyRound(c, d, e, f, g, h, a, b, words[i + 6U], constants[i + 6U]);
            applyRound(b, c, d, e, f, g, h, a, words[i + 7U], constants[i + 7U]);
        }
        // Feed the working state back into the chaining state. This addition is what makes the compression function
        // one-way; the wrap-around of the unsigned arithmetic is part of the algorithm.
        _state[0] += a;
        _state[1] += b;
        _state[2] += c;
        _state[3] += d;
        _state[4] += e;
        _state[5] += f;
        _state[6] += g;
        _state[7] += h;
    }

private:
    std::array<uint64_t, 8> _state{};  ///< The chaining state H0 to H7; its leading words become the digest.
    ByteArray<128> _buffer{};          ///< Collects bytes until a full 1024-bit block can be compressed.
    ByteIndex _bufferPosition{};       ///< The write-position in the buffer.
    uint64_t _bitLengthHigh{0};        ///< The upper 64 bits of the 128-bit message length in bits.
    uint64_t _bitLengthLow{0};         ///< The lower 64 bits of the 128-bit message length in bits.
    ByteArray<tDigestBytes> _digest{}; ///< A cache for the final digest.
    bool _hasDigest{false};            ///< Flag that the digest was calculated and is ready to use.
};

/// The SHA-384 hash algorithm.
/// 1024-bit blocks, 384-bit output.
using Sha2_384 = Sha2_64<48>;

/// The SHA-512 hash algorithm.
/// 1024-bit blocks, 512-bit output.
using Sha2_512 = Sha2_64<64>;

}
