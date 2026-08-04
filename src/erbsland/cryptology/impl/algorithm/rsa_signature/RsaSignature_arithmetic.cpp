// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "RsaSignature.hpp"

#include "../../../../mem/Byte.hpp"
#include "../../../../mem/ByteBlockEditor.hpp"
#include "../../../../text/Literals.hpp"
#include "../../../../unit/ByteIndex.hpp"

#include <cstdint>

namespace erbsland::cryptology::impl::rsa_signature {

using namespace text::literals;

auto numberFromBigEndian(const mem::ConstByteSpan bytes, const std::size_t wordCount) -> Number {
    // RFC 8017 section 4.2 OS2IP: accumulate the big-endian octets X into the represented integer x.
    auto result = Number{};
    result.count = wordCount;
    auto byteOffset = std::size_t{};
    for (auto source = bytes.size(); source > 0U; --source, ++byteOffset) {
        const auto wordIndex = byteOffset / 4U;
        if (wordIndex >= wordCount) {
            throwParseError("An RSA integer exceeds its fixed representation."_el);
        }
        const auto shift = static_cast<unsigned int>((byteOffset % 4U) * 8U);
        result.words[wordIndex] |= static_cast<uint32_t>(bytes[source - 1U].toUInt8()) << shift;
    }
    return result;
}

auto numberToBigEndian(const Number &value, const std::size_t length) -> mem::ByteBlock {
    // RFC 8017 section 4.1 I2OSP: emit the represented integer x as exactly xLen big-endian octets.
    auto result = mem::ByteBlockEditor{unit::ByteLength::fromSizeT(length)};
    for (auto destination = std::size_t{}; destination < length; ++destination) {
        const auto sourceOffset = length - destination - 1U;
        const auto wordIndex = sourceOffset / 4U;
        const auto shift = static_cast<unsigned int>((sourceOffset % 4U) * 8U);
        const auto byte = static_cast<uint8_t>((value.words[wordIndex] >> shift) & 0xffU);
        result.set(unit::ByteIndex{destination}, mem::Byte{byte});
    }
    return mem::ByteBlock{result};
}

auto compare(const Number &left, const Number &right) noexcept -> int {
    for (auto index = left.count; index > 0U; --index) {
        if (left.words[index - 1U] < right.words[index - 1U]) {
            return -1;
        }
        if (left.words[index - 1U] > right.words[index - 1U]) {
            return 1;
        }
    }
    return 0;
}

void subtract(Number &left, const Number &right) noexcept {
    auto borrow = uint64_t{};
    for (auto index = std::size_t{}; index < left.count; ++index) {
        const auto subtrahend = static_cast<uint64_t>(right.words[index]) + borrow;
        const auto minuend = static_cast<uint64_t>(left.words[index]);
        left.words[index] = static_cast<uint32_t>(minuend - subtrahend);
        borrow = minuend < subtrahend ? 1U : 0U;
    }
}

auto addModulo(const Number &left, const Number &right, const Number &modulus) noexcept -> Number {
    // RFC 8017 section 5.2.2 needs arithmetic modulo n; add two already reduced representatives without overflow.
    auto result = Number{};
    result.count = modulus.count;
    auto carry = uint64_t{};
    for (auto index = std::size_t{}; index < modulus.count; ++index) {
        const auto sum = static_cast<uint64_t>(left.words[index]) + right.words[index] + carry;
        result.words[index] = static_cast<uint32_t>(sum);
        carry = sum >> cWordBits;
    }
    // Both operands are below n, so their sum is below 2n and at most one reduction is required.
    // When the base-2^(32*count) addition carried, wrapped subtraction still represents `(left + right) - n`.
    if (carry != 0U || compare(result, modulus) >= 0) {
        subtract(result, modulus);
    }
    return result;
}

auto multiplyModulo(const Number &left, const Number &right, const Number &modulus) noexcept -> Number {
    // RFC 8017 section 5.2.2 computes `m = s^e mod n`. This supporting public-input multiplication maintains
    // `result = left * processedBits mod n` and `addend = left * 2^bit mod n`; fixed storage replaces BigInteger.
    auto result = Number{};
    result.count = modulus.count;
    auto addend = left;
    for (auto wordIndex = std::size_t{}; wordIndex < modulus.count; ++wordIndex) {
        auto bits = right.words[wordIndex];
        for (auto bit = std::size_t{}; bit < cWordBits; ++bit) {
            if ((bits & 1U) != 0U) {
                result = addModulo(result, addend, modulus);
            }
            bits >>= 1U;
            if (wordIndex + 1U != modulus.count || bit + 1U != cWordBits) {
                addend = addModulo(addend, addend, modulus);
            }
        }
    }
    return result;
}

auto rsaVerificationPrimitive(const PublicKeyData &key, const mem::ConstByteSpan signature)
    -> std::optional<mem::ByteBlock> {
    // RFC 8017 section 8.1.2 step 1 and section 8.2.2 step 1 require one signature of exactly k octets.
    if (signature.size() != key.encodedLength) {
        return std::nullopt;
    }

    // RFC 8017 sections 4.2 and 5.2.2 step 1: OS2IP converts S to `s`, which must satisfy 0 <= s < n.
    const auto signatureRepresentative = numberFromBigEndian(signature, key.modulus.count);
    if (compare(signatureRepresentative, key.modulus) >= 0) {
        return std::nullopt;
    }

    // RFC 8017 section 5.2.2 step 2 applies RSAVP1: `m = s^e mod n`.
    // Square-and-multiply branches only on the public exponent and operates solely on public values.
    auto messageRepresentative = Number{};
    messageRepresentative.count = key.modulus.count;
    messageRepresentative.words[0U] = 1U;
    for (const auto exponentByte : key.exponent.span()) {
        for (auto mask = uint8_t{0x80U}; mask != 0U; mask >>= 1U) {
            messageRepresentative = multiplyModulo(messageRepresentative, messageRepresentative, key.modulus);
            if ((exponentByte.toUInt8() & mask) != 0U) {
                messageRepresentative = multiplyModulo(messageRepresentative, signatureRepresentative, key.modulus);
            }
        }
    }

    // RFC 8017 sections 4.1 and 5.2.2 step 3: I2OSP converts `m` to the fixed k-octet encoded message.
    return numberToBigEndian(messageRepresentative, key.encodedLength);
}

}
