// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Hkdf.hpp"

#include "Hmac.hpp"

#include "impl/authentication/HmacWorkerFactory.hpp"
#include "keys/KeyAgreementSharedSecret.hpp"

#include "../err/LogicError.hpp"
#include "../err/ParameterError.hpp"
#include "../mem/ByteArray.hpp"
#include "../mem/ByteBlock.hpp"
#include "../mem/ByteBlockEditor.hpp"
#include "../text/Literals.hpp"
#include "../unit/ByteIndex.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace erbsland::cryptology {

using namespace text::literals;

Hkdf::Hkdf(const HashAlgorithm algorithm) : _algorithm{algorithm} {
    // RFC 5869 is hash-independent; this library intentionally exposes only its reviewed SHA-256/SHA-384 instances.
    if (!impl::isHmacAlgorithmSupported(algorithm)) {
        throw err::ParameterError{"HKDF supports only SHA-256 and SHA-384."_el, "algorithm"_el};
    }
}

auto Hkdf::extract(const mem::ConstByteSpan inputKeyMaterial, const mem::ConstByteSpan salt) const -> mem::ByteBlock {
    // RFC 5869 section 2.2: an absent salt is HashLen zero bytes, not an empty HMAC key.
    auto zeroSalt = mem::ByteBlock{};
    if (salt.empty()) {
        zeroSalt = mem::ByteBlock{_algorithm.digestSize()};
        zeroSalt.markAsSensitive();
    }

    // RFC 5869 section 2.2: PRK = HMAC-Hash(salt, IKM).
    auto hmac = Hmac{_algorithm, salt.empty() ? zeroSalt.span() : salt};
    hmac.update(inputKeyMaterial);
    auto result = hmac.finalize();
    result.markAsSensitive();
    hmac.secureErase();
    zeroSalt.secureErase();
    return result;
}

auto Hkdf::extract(const KeyAgreementSharedSecret &sharedSecret, const mem::ConstByteSpan salt) const
    -> mem::ByteBlock {
    if (sharedSecret.isEmpty()) {
        throw err::LogicError{"A key-agreement shared secret is required for HKDF extraction."_el};
    }
    auto result = mem::ByteBlock{};
    // RFC 5869 section 2.2: use the shared secret as IKM only within scoped plaintext access. ProtectedByteBlock
    // erases the temporary after HMAC has copied the IKM into its own guarded state, including on exceptions.
    sharedSecret._data.withUnprotectedData(
        [&](const mem::ConstByteSpan data) -> void { result = extract(data, salt); });
    return result;
}

auto Hkdf::expand(
    const mem::ConstByteSpan pseudoRandomKey, const mem::ConstByteSpan info, const unit::ByteLength outputLength) const
    -> mem::ByteBlock {
    const auto digestSize = _algorithm.digestSize().toSizeT();
    // RFC 5869 section 2.3 defines the PRK input as at least HashLen bytes. Enforce this before allocating output.
    if (pseudoRandomKey.size() < digestSize) {
        throw err::ParameterError{
            "The HKDF pseudorandom key is shorter than the digest size."_el, "pseudoRandomKey"_el};
    }
    const auto maximumOutputLength = unit::ByteLength::fromSizeT(digestSize * 255U);
    // RFC 5869 section 2.3 limits L to 255 * HashLen because the block index is encoded in one octet.
    if (outputLength > maximumOutputLength) {
        throw err::ParameterError{"The HKDF output length exceeds 255 digest blocks."_el, "outputLength"_el};
    }

    auto result = mem::ByteBlockEditor{outputLength};
    result.markAsSensitive();
    // T(i) is secret output keying material. Marking it sensitive makes replacement erase the preceding allocation.
    auto previous = mem::ByteBlock{};
    previous.markAsSensitive();
    const auto requestedLength = outputLength.toSizeT();
    // RFC 5869 section 2.3: N = ceil(L / HashLen). Avoid subtracting one for the valid empty-output case.
    const auto blockCount = requestedLength == 0U ? 0U : ((requestedLength - 1U) / digestSize) + 1U;
    auto offset = std::size_t{};
    for (auto blockIndex = std::size_t{1U}; blockIndex <= blockCount; ++blockIndex) {
        // RFC 5869 section 2.3: T(0) is empty and T(i) = HMAC-Hash(PRK, T(i-1) || info || i).
        auto hmac = Hmac{_algorithm, pseudoRandomKey};
        if (!previous.isEmpty()) {
            hmac.update(previous);
        }
        hmac.update(info);
        const auto counter = mem::ByteArray<1U>{mem::Byte{static_cast<uint8_t>(blockIndex)}};
        hmac.update(counter.span());
        auto block = hmac.finalize();
        block.markAsSensitive();
        hmac.secureErase();

        // RFC 5869 section 2.3: OKM = first L octets of T(1) || T(2) || ... || T(N).
        const auto count = std::min(digestSize, requestedLength - offset);
        result.overwrite(unit::ByteIndex::fromSizeT(offset), block.span().first(count));
        previous = std::move(block);
        offset += count;
    }
    return result;
}

}
