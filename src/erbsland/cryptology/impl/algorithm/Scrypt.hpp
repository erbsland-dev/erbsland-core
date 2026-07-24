// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HmacSha256.hpp"

#include "../SecureEraseGuard.hpp"

#include "../../../err/ParameterError.hpp"
#include "../../../math/IntegerBitOperations.hpp"
#include "../../../mem/ByteArray.hpp"
#include "../../../mem/ByteBlock.hpp"
#include "../../../mem/ByteBlockEditor.hpp"
#include "../../../mem/ByteBuffer.hpp"
#include "../../../mem/ByteSpan.hpp"
#include "../../../mem/impl/ByteIntegerAccess.hpp"
#include "../../../mem/impl/ByteSequenceOperations.hpp"
#include "../../../mem/impl/SecureErase.hpp"
#include "../../../unit/ByteLength.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>

namespace erbsland::cryptology::impl {

// algorithms are never included in the public API, therefore using these namespaces never leaks.
using namespace erbsland::unit;
using namespace erbsland::mem;

/// The Salsa20/8 core used by scrypt's BlockMix operation.
///
/// This is not the Salsa20 stream cipher interface. It applies four column/row double rounds to one 64-byte block and
/// adds the original words back to the result, exactly as specified by RFC 7914 section 3.
/// @param block The 64-byte block modified in place.
/// @tested{PasswordPrimitiveTest}
inline void salsa20_8(ByteArray<64> &block) noexcept {
    // RFC 7914 section 3 interprets the input as sixteen little-endian 32-bit words.
    auto input = std::array<uint32_t, 16>{};
    auto x = std::array<uint32_t, 16>{};
    for (auto i = std::size_t{0}; i < 16U; ++i) {
        input[i] = block.getInteger<uint32_t>(ByteIndex::fromSizeT(i * sizeof(uint32_t)), Endianness::Little);
    }
    x = input;
    // RFC 7914 reproduces Salsa20/8's eight rounds as four column/row double rounds. The 7, 9, 13, and 18-bit
    // rotations are Salsa20's fixed quarter-round constants; each macro use is one add-rotate-XOR step.
#define EL_SALSA_STEP(a, b, rotation) a ^= math::rotateLeft(static_cast<uint32_t>(b), rotation)
    for (auto round = std::size_t{0}; round < 8U; round += 2U) {
        // Column round over the 4x4 word matrix.
        EL_SALSA_STEP(x[4], x[0] + x[12], 7);
        EL_SALSA_STEP(x[8], x[4] + x[0], 9);
        EL_SALSA_STEP(x[12], x[8] + x[4], 13);
        EL_SALSA_STEP(x[0], x[12] + x[8], 18);
        EL_SALSA_STEP(x[9], x[5] + x[1], 7);
        EL_SALSA_STEP(x[13], x[9] + x[5], 9);
        EL_SALSA_STEP(x[1], x[13] + x[9], 13);
        EL_SALSA_STEP(x[5], x[1] + x[13], 18);
        EL_SALSA_STEP(x[14], x[10] + x[6], 7);
        EL_SALSA_STEP(x[2], x[14] + x[10], 9);
        EL_SALSA_STEP(x[6], x[2] + x[14], 13);
        EL_SALSA_STEP(x[10], x[6] + x[2], 18);
        EL_SALSA_STEP(x[3], x[15] + x[11], 7);
        EL_SALSA_STEP(x[7], x[3] + x[15], 9);
        EL_SALSA_STEP(x[11], x[7] + x[3], 13);
        EL_SALSA_STEP(x[15], x[11] + x[7], 18);
        // Row round over the same matrix.
        EL_SALSA_STEP(x[1], x[0] + x[3], 7);
        EL_SALSA_STEP(x[2], x[1] + x[0], 9);
        EL_SALSA_STEP(x[3], x[2] + x[1], 13);
        EL_SALSA_STEP(x[0], x[3] + x[2], 18);
        EL_SALSA_STEP(x[6], x[5] + x[4], 7);
        EL_SALSA_STEP(x[7], x[6] + x[5], 9);
        EL_SALSA_STEP(x[4], x[7] + x[6], 13);
        EL_SALSA_STEP(x[5], x[4] + x[7], 18);
        EL_SALSA_STEP(x[11], x[10] + x[9], 7);
        EL_SALSA_STEP(x[8], x[11] + x[10], 9);
        EL_SALSA_STEP(x[9], x[8] + x[11], 13);
        EL_SALSA_STEP(x[10], x[9] + x[8], 18);
        EL_SALSA_STEP(x[12], x[15] + x[14], 7);
        EL_SALSA_STEP(x[13], x[12] + x[15], 9);
        EL_SALSA_STEP(x[14], x[13] + x[12], 13);
        EL_SALSA_STEP(x[15], x[14] + x[13], 18);
    }
#undef EL_SALSA_STEP
    // Salsa20/8 finishes with feed-forward addition of the original words, then serializes little-endian.
    for (auto i = std::size_t{0}; i < 16U; ++i) {
        const auto word = static_cast<uint32_t>(x[i] + input[i]);
        static_cast<void>(block.setInteger(ByteIndex::fromSizeT(i * sizeof(uint32_t)), word, Endianness::Little));
    }
    mem::impl::secureErase(std::as_writable_bytes(std::span{input}));
    mem::impl::secureErase(std::as_writable_bytes(std::span{x}));
}

/// Apply the Salsa20/8 core at an explicit writable-span boundary.
/// @tested{PasswordPrimitiveTest}
inline void salsa20_8(const FixedByteSpan<64> block) noexcept {
    auto value = ByteArray<64>{};
    [[maybe_unused]] const auto valueErase = SecureEraseGuard{value};
    value.overwrite(ConstByteSpan{block});
    salsa20_8(value);
    static_cast<void>(mem::impl::overwrite(block, ByteRange::all(), value.span()));
}

/// Apply scrypt BlockMix using Salsa20/8.
///
/// Successive Salsa results are first stored in Y, then all even results followed by all odd results are written back
/// to B. This permutation is the source of scrypt's sequential memory access pattern.
/// @param block The 128*r-byte block B modified in place.
/// @param r The positive scrypt block-size parameter.
/// @throws err::ParameterError If `r` is zero or does not match the supplied block length.
/// @tested{PasswordPrimitiveTest}
inline void scryptBlockMix(ByteBuffer &block, const uint32_t r) {
    const auto expectedLength = static_cast<std::size_t>(128U) * r;
    if (r == 0U || block.length().toSizeT() != expectedLength) {
        throw err::ParameterError{"Invalid scrypt BlockMix dimensions", "r"};
    }
    auto x = ByteArray<64>{};
    [[maybe_unused]] const auto xErase = SecureEraseGuard{x};
    auto y = ByteBuffer{ByteLength::fromSizeT(expectedLength)};
    [[maybe_unused]] const auto yErase = SecureEraseGuard{y};
    // RFC 7914 section 4 initializes X from the final 64-byte input chunk B[2r-1].
    x.overwrite(block.span(ByteIndex::fromSizeT(expectedLength - 64U), ByteLength{64U}));
    // For each chunk, compute X = Salsa20/8(X XOR B[i]) and retain the result as Y[i].
    for (auto i = std::size_t{0}; i < static_cast<std::size_t>(2U) * r; ++i) {
        static_cast<void>(x.xorWith(block.span(ByteIndex::fromSizeT(i * 64U), ByteLength{64U})));
        salsa20_8(x);
        y.overwrite(ByteIndex::fromSizeT(i * 64U), x.span());
    }
    // RFC 7914's output permutation concatenates the even Y chunks followed by the odd Y chunks.
    for (auto i = std::size_t{0}; i < r; ++i) {
        block.overwrite(ByteIndex::fromSizeT(i * 64U), y.span(ByteIndex::fromSizeT(i * 2U * 64U), ByteLength{64U}));
        block.overwrite(
            ByteIndex::fromSizeT((static_cast<std::size_t>(r) + i) * 64U),
            y.span(ByteIndex::fromSizeT((i * 2U + 1U) * 64U), ByteLength{64U}));
    }
}

/// Apply scrypt ROMix to one 128*r-byte chunk.
/// @param block The 128*r-byte chunk B modified in place.
/// @param cost The scrypt N parameter; the caller must provide a power of two greater than one.
/// @param r The positive scrypt block-size parameter.
/// @warning The caller must validate dimensions and allocation limits before calling this primitive.
/// @tested{PasswordPrimitiveTest}
inline void scryptRomix(ByteBuffer &block, const uint64_t cost, const uint32_t r) {
    const auto blockLength = static_cast<std::size_t>(128U) * r;
    auto x = ByteBuffer{block.span()};
    [[maybe_unused]] const auto xErase = SecureEraseGuard{x};
    auto v = ByteBuffer{ByteLength::fromSizeT(static_cast<std::size_t>(cost) * blockLength)};
    [[maybe_unused]] const auto vErase = SecureEraseGuard{v};
    // RFC 7914 section 5 first fills V sequentially: V[i] = X, then X = BlockMix(X).
    for (auto i = uint64_t{0}; i < cost; ++i) {
        v.overwrite(ByteIndex::fromSizeT(static_cast<std::size_t>(i) * blockLength), x.span());
        scryptBlockMix(x, r);
    }
    // The second loop selects an earlier state with Integerify(X) mod N, XORs it into X, then applies BlockMix.
    // This data-dependent walk forces an implementation to retain or recompute the large V table.
    for (auto i = uint64_t{0}; i < cost; ++i) {
        // Integerify reads the first 64-bit little-endian word of the final 64-byte chunk X[2r-1].
        const auto integerOffset = blockLength - 64U;
        const auto integer = x.getInteger<uint64_t>(ByteIndex::fromSizeT(integerOffset));
        // N is required to be a power of two, so `integer & (N-1)` is exactly `integer mod N`.
        const auto selected = integer & (cost - 1U);
        const auto selectedOffset = static_cast<std::size_t>(selected) * blockLength;
        x.xorWith(ByteRange::all(), v.span(ByteIndex::fromSizeT(selectedOffset), ByteLength::fromSizeT(blockLength)));
        scryptBlockMix(x, r);
    }
    // Copy the final X back into the caller's B chunk.
    block.overwrite(x.span());
}

/// Derive bytes with scrypt.
///
/// PBKDF2-HMAC-SHA-256 expands the password into `p` chunks. ROMix makes each chunk depend on `N` earlier states, and
/// a final single-iteration PBKDF2 compresses the mixed state to the requested output.
///
/// Source: https://www.rfc-editor.org/rfc/rfc7914.html
/// @param password The password bytes.
/// @param salt The public salt.
/// @param cost The CPU/memory cost N, which must be a power of two greater than one.
/// @param r The positive block-size parameter.
/// @param parallelization The positive parallelization parameter p.
/// @param outputLength The requested derived-key length.
/// @return The derived bytes in marked storage.
/// @throws err::ParameterError If parameters are invalid or their dimensions overflow `std::size_t`.
/// @tested{PasswordPrimitiveTest}
[[nodiscard]] inline auto scrypt(
    const ConstByteSpan password,
    const ConstByteSpan salt,
    const uint64_t cost,
    const uint32_t r,
    const uint32_t parallelization,
    const std::size_t outputLength) -> ByteBlockEditor {
    if (cost <= 1U || (cost & (cost - 1U)) != 0U) {
        throw err::ParameterError{"scrypt N must be a power of two greater than one", "cost"};
    }
    if (r == 0U || parallelization == 0U) {
        throw err::ParameterError{"scrypt r and p must be positive", "parameters"};
    }
    constexpr auto max = std::numeric_limits<std::size_t>::max();
    if constexpr (sizeof(std::size_t) <= sizeof(uint32_t)) {
        if (r > max / 128U) {
            throw err::ParameterError{"scrypt dimensions overflow", "parameters"};
        }
    }
    if (parallelization > max / (static_cast<std::size_t>(128U) * r)) {
        throw err::ParameterError{"scrypt dimensions overflow", "parameters"};
    }
    const auto blockLength = static_cast<std::size_t>(128U) * r;
    const auto expandedLength = blockLength * parallelization;
    if (cost > max / blockLength) {
        throw err::ParameterError{"scrypt memory size overflows", "cost"};
    }
    // RFC 7914 section 6 step 1: B = PBKDF2-HMAC-SHA-256(P, S, 1, p * 128 * r).
    auto expanded = pbkdf2HmacSha256(password, salt, 1U, expandedLength);
    // Step 2: apply ROMix independently to each of the p consecutive 128*r-byte chunks.
    for (auto chunk = std::size_t{0}; chunk < parallelization; ++chunk) {
        const auto chunkRange =
            ByteRange{ByteIndex::fromSizeT(chunk * blockLength), ByteLength::fromSizeT(blockLength)};
        auto scratch = ByteBuffer{expanded.span(chunkRange)};
        [[maybe_unused]] const auto scratchErase = SecureEraseGuard{scratch};
        scryptRomix(scratch, cost, r);
        expanded.overwrite(chunkRange, scratch.span());
    }
    // Step 3: DK = PBKDF2-HMAC-SHA-256(P, mixed B, 1, dkLen).
    return pbkdf2HmacSha256(password, expanded.span(), 1U, outputLength);
}

}
