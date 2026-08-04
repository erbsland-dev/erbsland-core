// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HmacAlgorithm.hpp"
#include "Sha2.hpp"

#include "../../../err/ParameterError.hpp"
#include "../../../mem/ByteBlock.hpp"
#include "../../../mem/ByteBlockEditor.hpp"
#include "../../../mem/ByteSpan.hpp"
#include "../../../text/Literals.hpp"
#include "../../../unit/ByteLength.hpp"
#include "../../../unit/ByteRange.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <limits>

namespace erbsland::cryptology::impl {

// algorithms are never included in the public API, therefore using these namespaces never leaks.
using namespace erbsland::unit;
using namespace erbsland::mem;
using namespace text::literals;

/// Calculate HMAC-SHA-256.
///
/// RFC 2104 first normalizes the key to SHA-256's 64-byte block size, then hashes an inner and outer keyed message.
/// The two different XOR pads prevent the inner digest from being used as a plain length-extension target.
///
/// Source: https://www.rfc-editor.org/rfc/rfc2104.html
/// @param key The secret key bytes.
/// @param messageParts The message spans, hashed as one concatenated message without creating an ordinary copy.
/// @return The 32-byte MAC in marked storage.
/// @tested{PasswordPrimitiveTest HashPrimitiveFullValidationTest}
[[nodiscard]] inline auto hmacSha256(const ConstByteSpan key, const std::initializer_list<ConstByteSpan> messageParts)
    -> ByteBlock {
    // SHA-256's 64-bit bit-length field includes the preloaded 64-byte inner pad; reserve that block in the limit.
    constexpr auto maximumMessageLength = (uint64_t{1U} << 61U) - 65U;
    auto hmac = HmacAlgorithm<Sha2_256, 64U, maximumMessageLength>{key};
    for (const auto part : messageParts) {
        hmac.update(part);
    }
    auto digest = hmac.finalize();
    // PBKDF2 treats the HMAC result as secret keying material, so transfer it to marked storage before erasing the
    // stack-resident digest returned by the generic HMAC core.
    auto result = ByteBlock{digest};
    digest.secureErase();
    result.markAsSensitive();
    return result;
}

/// PBKDF2 using HMAC-SHA-256 as its pseudorandom function.
///
/// Each output block is U1 xor U2 ... Uc, where U1 authenticates `salt || INT(block)` and every later U value
/// authenticates the preceding U. This function is an internal building block for scrypt, not a public password
/// hashing option.
///
/// Source: https://www.rfc-editor.org/rfc/rfc8018.html (section 5.2)
/// @param password The pseudorandom-function key.
/// @param salt The public salt.
/// @param iterations The positive PBKDF2 iteration count.
/// @param outputLength The requested derived-key length.
/// @return The derived bytes in marked storage.
/// @throws err::ParameterError If the iteration count is zero or the output requires more than 2^32-1 blocks.
/// @tested{PasswordPrimitiveTest}
[[nodiscard]] inline auto pbkdf2HmacSha256(
    const ConstByteSpan password, const ConstByteSpan salt, const uint32_t iterations, const std::size_t outputLength)
    -> ByteBlockEditor {
    if (iterations == 0U) {
        throw err::ParameterError{"PBKDF2 iteration count must be positive."_el, "iterations"_el};
    }
    const auto blockCount = (outputLength + 31U) / 32U;
    if (blockCount > std::numeric_limits<uint32_t>::max()) {
        throw err::ParameterError{"PBKDF2 output length is too large."_el, "outputLength"_el};
    }
    auto result = ByteBlockEditor{ByteLength::fromSizeT(outputLength)};
    result.markAsSensitive();
    auto saltAndIndex = ByteBlockEditor::fromSpan(salt);
    saltAndIndex.resize(ByteLength::fromSizeT(salt.size() + 4U));

    // RFC 8018 section 5.2 numbers output blocks from one and encodes INT(i) as four big-endian bytes.
    for (auto block = uint32_t{1U}; block <= static_cast<uint32_t>(blockCount); ++block) {
        saltAndIndex.setIntegerOrThrow(ByteIndex::fromSizeT(salt.size()), block, Endianness::Big);
        // U1 = PRF(P, S || INT(i)); T starts as U1.
        auto current = hmacSha256(password, {saltAndIndex.span()});
        auto accumulated = ByteBlockEditor{ByteBlock{current}};
        // Uj = PRF(P, Uj-1), and T is the XOR of every U value through the configured count c.
        for (auto iteration = uint32_t{1U}; iteration < iterations; ++iteration) {
            current = hmacSha256(password, {current.span()});
            // Both values are one SHA-256 digest; fail rather than partially XOR if that invariant is ever broken.
            accumulated.xorWithOrThrow(current);
        }
        // Concatenate T1, T2, ... and truncate only the final block to the requested derived-key length.
        const auto offset = static_cast<std::size_t>(block - 1U) * 32U;
        const auto count = std::min(std::size_t{32U}, outputLength - offset);
        result.overwrite(ByteRange{ByteIndex::fromSizeT(offset), ByteLength::fromSizeT(count)}, accumulated.span());
    }
    return result;
}

}
