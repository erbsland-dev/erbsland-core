// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Number.hpp"

#include "../../../../core/Application.hpp"
#include "../../../../err/ParseError.hpp"
#include "../../../../mem/Byte.hpp"
#include "../../../../mem/ByteBlockEditor.hpp"
#include "../../../../mem/SecureErase.hpp"
#include "../../../../random/Random.hpp"
#include "../../../../text/Literals.hpp"
#include "../../../../unit/ByteIndex.hpp"
#include "../../../../unit/ByteLength.hpp"
#include "../../../CryptologyError.hpp"
#include "../../SecureEraseGuard.hpp"

#include <algorithm>
#include <array>
#include <bit>

namespace erbsland::cryptology::impl::rsa_signature {

using namespace mem;
using namespace text::literals;
using namespace unit;

constexpr auto cPublicExponent = uint32_t{65537U};
constexpr auto cMaximumPrimeAttempts = std::size_t{1U << 20U};
constexpr auto cMillerRabinRounds = std::size_t{25U};
constexpr auto cMaximumBaseAttempts = std::size_t{128U};
constexpr auto cTrialPrimes = std::array<uint32_t, 53U>{
    3U,
    5U,
    7U,
    11U,
    13U,
    17U,
    19U,
    23U,
    29U,
    31U,
    37U,
    41U,
    43U,
    47U,
    53U,
    59U,
    61U,
    67U,
    71U,
    73U,
    79U,
    83U,
    89U,
    97U,
    101U,
    103U,
    107U,
    109U,
    113U,
    127U,
    131U,
    137U,
    139U,
    149U,
    151U,
    157U,
    163U,
    167U,
    173U,
    179U,
    181U,
    191U,
    193U,
    197U,
    199U,
    211U,
    223U,
    227U,
    229U,
    233U,
    239U,
    241U,
    251U};

auto Number::fromValue(const uint32_t value, const std::size_t wordCount) noexcept -> Number {
    auto result = Number{};
    result._wordCount = wordCount;
    result._words[0U] = value;
    return result;
}

auto Number::fromBigEndian(const ConstByteSpan bytes, const std::size_t wordCount) -> Number {
    auto result = Number{};
    result._wordCount = wordCount;
    auto byteOffset = std::size_t{};
    for (auto source = bytes.size(); source > 0U; --source, ++byteOffset) {
        const auto wordIndex = byteOffset / 4U;
        if (wordIndex >= wordCount) {
            throw err::ParseError{"An RSA integer exceeds its fixed representation."_el};
        }
        const auto shift = static_cast<unsigned int>((byteOffset % 4U) * 8U);
        result._words[wordIndex] |= static_cast<uint32_t>(bytes[source - 1U].toUInt8()) << shift;
    }
    return result;
}

auto Number::generatePrime(const std::size_t bits) -> Number {
    const auto byteLength = bits / 8U;
    const auto wordCount = bits / cWordBits;
    for (auto attempt = std::size_t{}; attempt < cMaximumPrimeAttempts; ++attempt) {
        auto bytes = core::application().secureRandom().buildByteBlock(ByteLength{byteLength});
        bytes.markAsSensitive();
        const auto bytesEraseGuard = SecureEraseGuard{bytes};
        auto editor = ByteBlockEditor{bytes};
        editor.markAsSensitive();
        const auto editorEraseGuard = SecureEraseGuard{editor};
        editor.set(ByteIndex::zero(), Byte{static_cast<uint8_t>(editor.get(ByteIndex::zero()).toUInt8() | 0xc0U)});
        editor.set(
            ByteIndex{byteLength - 1U},
            Byte{static_cast<uint8_t>(editor.get(ByteIndex{byteLength - 1U}).toUInt8() | 1U)});
        auto candidate = fromBigEndian(editor.span(), wordCount);
        auto candidateEraseGuard = SecureEraseGuard{candidate};
        if (candidate.modulo(cPublicExponent) != 1U && candidate.isProbablePrime()) {
            candidateEraseGuard.release();
            return candidate;
        }
    }
    throw CryptologyError{"RSA probable-prime generation exceeded its fixed candidate bound."_el};
}

auto Number::word(const std::size_t index) const noexcept -> uint32_t {
    return index < _wordCount ? _words[index] : uint32_t{};
}

auto Number::isZero() const noexcept -> bool {
    auto combined = uint32_t{};
    for (auto index = std::size_t{}; index < _wordCount; ++index) {
        combined |= _words[index];
    }
    return combined == 0U;
}

auto Number::isOne() const noexcept -> bool {
    auto difference = _words[0U] ^ 1U;
    for (auto index = std::size_t{1U}; index < _wordCount; ++index) {
        difference |= _words[index];
    }
    return difference == 0U;
}

auto Number::isOdd() const noexcept -> bool {
    return _wordCount != 0U && (_words[0U] & 1U) != 0U;
}

auto Number::bitLength() const noexcept -> std::size_t {
    for (auto index = _wordCount; index-- > 0U;) {
        if (_words[index] != 0U) {
            return index * cWordBits + static_cast<std::size_t>(std::bit_width(_words[index]));
        }
    }
    return 0U;
}

auto Number::compare(const Number &other) const noexcept -> int {
    const auto width = std::max(_wordCount, other._wordCount);
    for (auto index = width; index > 0U; --index) {
        if (word(index - 1U) < other.word(index - 1U)) {
            return -1;
        }
        if (word(index - 1U) > other.word(index - 1U)) {
            return 1;
        }
    }
    return 0;
}

auto Number::isEqual(const Number &other, const std::size_t wordCount) const noexcept -> bool {
    auto difference = uint32_t{};
    for (auto index = std::size_t{}; index < wordCount; ++index) {
        difference |= word(index) ^ other.word(index);
    }
    return difference == 0U;
}

auto Number::toBigEndian(const std::size_t length) const -> ByteBlock {
    auto result = ByteBlockEditor{ByteLength::fromSizeT(length)};
    for (auto destination = std::size_t{}; destination < length; ++destination) {
        const auto sourceOffset = length - destination - 1U;
        const auto wordIndex = sourceOffset / 4U;
        const auto shift = static_cast<unsigned int>((sourceOffset % 4U) * 8U);
        result.set(ByteIndex{destination}, Byte{static_cast<uint8_t>((word(wordIndex) >> shift) & 0xffU)});
    }
    return ByteBlock{result};
}

auto Number::padded(const std::size_t wordCount) const noexcept -> Number {
    auto result = *this;
    result._wordCount = wordCount;
    return result;
}

void Number::subtract(const Number &other) noexcept {
    auto borrow = uint64_t{};
    for (auto index = std::size_t{}; index < _wordCount; ++index) {
        const auto subtrahend = uint64_t{other.word(index)} + borrow;
        const auto minuend = uint64_t{_words[index]};
        _words[index] = static_cast<uint32_t>(minuend - subtrahend);
        borrow = minuend < subtrahend ? 1U : 0U;
    }
}

auto Number::subtracted(const Number &other) const noexcept -> Number {
    auto result = *this;
    result.subtract(other);
    return result;
}

auto Number::subtractOne() const noexcept -> Number {
    auto result = *this;
    auto borrow = uint64_t{1U};
    for (auto index = std::size_t{}; index < result._wordCount; ++index) {
        const auto current = uint64_t{result._words[index]};
        result._words[index] = static_cast<uint32_t>(current - borrow);
        borrow = current < borrow ? 1U : 0U;
    }
    return result;
}

auto Number::shiftedRight() const noexcept -> Number {
    auto result = *this;
    auto carry = uint32_t{};
    for (auto index = result._wordCount; index-- > 0U;) {
        const auto nextCarry = result._words[index] << 31U;
        result._words[index] = (result._words[index] >> 1U) | carry;
        carry = nextCarry;
    }
    return result;
}

auto Number::modulo(const uint32_t divisor) const noexcept -> uint32_t {
    auto remainder = uint64_t{};
    for (auto index = _wordCount; index-- > 0U;) {
        remainder = ((remainder << cWordBits) | _words[index]) % divisor;
    }
    return static_cast<uint32_t>(remainder);
}

auto Number::multipliedAndIncremented(const uint32_t multiplier) const noexcept -> Number {
    auto result = Number{};
    result._wordCount = _wordCount + 1U;
    auto carry = uint64_t{1U};
    for (auto index = std::size_t{}; index < _wordCount; ++index) {
        const auto product = uint64_t{_words[index]} * multiplier + carry;
        result._words[index] = static_cast<uint32_t>(product);
        carry = product >> cWordBits;
    }
    result._words[_wordCount] = static_cast<uint32_t>(carry);
    return result;
}

auto Number::divided(const uint32_t divisor, const std::size_t resultWords) const noexcept -> Number {
    auto result = Number{};
    result._wordCount = resultWords;
    auto remainder = uint64_t{};
    for (auto index = _wordCount; index-- > 0U;) {
        const auto current = (remainder << cWordBits) | _words[index];
        if (index < resultWords) {
            result._words[index] = static_cast<uint32_t>(current / divisor);
        }
        remainder = current % divisor;
    }
    return result;
}

auto Number::added(const Number &other, const std::size_t resultWords) const noexcept -> Number {
    auto result = Number{};
    result._wordCount = resultWords;
    auto carry = uint64_t{};
    for (auto index = std::size_t{}; index < resultWords; ++index) {
        const auto sum = uint64_t{word(index)} + other.word(index) + carry;
        result._words[index] = static_cast<uint32_t>(sum);
        carry = sum >> cWordBits;
    }
    return result;
}

auto Number::multiplied(const Number &other, const std::size_t resultWords) const noexcept -> Number {
    auto result = Number{};
    result._wordCount = resultWords;
    for (auto leftIndex = std::size_t{}; leftIndex < _wordCount; ++leftIndex) {
        auto carry = uint64_t{};
        for (auto rightIndex = std::size_t{}; rightIndex < other._wordCount; ++rightIndex) {
            const auto resultIndex = leftIndex + rightIndex;
            if (resultIndex >= resultWords) {
                continue;
            }
            const auto product =
                uint64_t{_words[leftIndex]} * other._words[rightIndex] + result._words[resultIndex] + carry;
            result._words[resultIndex] = static_cast<uint32_t>(product);
            carry = product >> cWordBits;
        }
        if (leftIndex + other._wordCount < resultWords) {
            result._words[leftIndex + other._wordCount] = static_cast<uint32_t>(carry);
        }
    }
    return result;
}

auto Number::addedModulo(const Number &other, const Number &modulus) const noexcept -> Number {
    auto result = added(other, modulus._wordCount);
    auto carry = uint64_t{};
    for (auto index = std::size_t{}; index < modulus._wordCount; ++index) {
        const auto sum = uint64_t{_words[index]} + other._words[index] + carry;
        carry = sum >> cWordBits;
    }
    if (carry != 0U || result.compare(modulus) >= 0) {
        result.subtract(modulus);
    }
    return result;
}

auto Number::multipliedModulo(const Number &other, const Number &modulus) const noexcept -> Number {
    auto result = fromValue(0U, modulus._wordCount);
    auto addend = *this;
    for (auto wordIndex = std::size_t{}; wordIndex < modulus._wordCount; ++wordIndex) {
        auto bits = other._words[wordIndex];
        for (auto bit = std::size_t{}; bit < cWordBits; ++bit) {
            if ((bits & 1U) != 0U) {
                result = result.addedModulo(addend, modulus);
            }
            bits >>= 1U;
            if (wordIndex + 1U != modulus._wordCount || bit + 1U != cWordBits) {
                addend = addend.addedModulo(addend, modulus);
            }
        }
    }
    return result;
}

auto Number::randomMillerRabinBase() const -> Number {
    const auto byteLength = _wordCount * sizeof(uint32_t);
    const auto one = fromValue(1U, _wordCount);
    for (auto attempt = std::size_t{}; attempt < cMaximumBaseAttempts; ++attempt) {
        auto bytes = core::application().secureRandom().buildByteBlock(ByteLength{byteLength});
        bytes.markAsSensitive();
        const auto bytesEraseGuard = SecureEraseGuard{bytes};
        auto result = fromBigEndian(bytes.span(), _wordCount);
        auto resultEraseGuard = SecureEraseGuard{result};
        if (result.compare(one) > 0 && result.compare(*this) < 0) {
            resultEraseGuard.release();
            return result;
        }
    }
    throw CryptologyError{"RSA primality testing could not sample a bounded Miller-Rabin base."_el};
}

auto Number::isProbablePrime() const -> bool {
    for (const auto prime : cTrialPrimes) {
        if (modulo(prime) == 0U) {
            return false;
        }
    }
    auto candidateMinusOne = subtractOne();
    const auto candidateMinusOneEraseGuard = SecureEraseGuard{candidateMinusOne};
    auto oddPart = candidateMinusOne;
    const auto oddPartEraseGuard = SecureEraseGuard{oddPart};
    auto powerOfTwo = std::size_t{};
    while (!oddPart.isOdd()) {
        oddPart = oddPart.shiftedRight();
        ++powerOfTwo;
    }
    const auto one = fromValue(1U, _wordCount);
    for (auto round = std::size_t{}; round < cMillerRabinRounds; ++round) {
        auto base = candidateMinusOne.randomMillerRabinBase();
        const auto baseEraseGuard = SecureEraseGuard{base};
        auto witness = base.poweredModuloSecret(oddPart, *this);
        const auto witnessEraseGuard = SecureEraseGuard{witness};
        if (witness.compare(one) == 0 || witness.compare(candidateMinusOne) == 0) {
            continue;
        }
        auto accepted = false;
        for (auto square = std::size_t{1U}; square < powerOfTwo; ++square) {
            auto squared = witness.multipliedModuloSecret(witness, *this);
            const auto squaredEraseGuard = SecureEraseGuard{squared};
            witness.secureErase();
            witness = squared;
            if (witness.compare(candidateMinusOne) == 0) {
                accepted = true;
                break;
            }
            if (witness.compare(one) == 0) {
                return false;
            }
        }
        if (!accepted) {
            return false;
        }
    }
    return true;
}

auto Number::inversePublicExponent() const noexcept -> Number {
    const auto value = modulo(cPublicExponent);
    auto oldR = int64_t{cPublicExponent};
    auto newR = int64_t{value};
    auto oldT = int64_t{};
    auto newT = int64_t{1U};
    while (newR != 0) {
        const auto quotient = oldR / newR;
        const auto nextR = oldR - quotient * newR;
        oldR = newR;
        newR = nextR;
        const auto nextT = oldT - quotient * newT;
        oldT = newT;
        newT = nextT;
    }
    if (oldT < 0) {
        oldT += cPublicExponent;
    }
    const auto inverse = static_cast<uint32_t>(oldT);
    const auto multiplier = inverse == 0U ? 0U : cPublicExponent - inverse;
    auto numerator = multipliedAndIncremented(multiplier);
    const auto numeratorEraseGuard = SecureEraseGuard{numerator};
    return numerator.divided(cPublicExponent, _wordCount);
}

auto Number::isSeparatedFrom(const Number &other, const std::size_t primeBits) const noexcept -> bool {
    auto difference = compare(other) >= 0 ? subtracted(other) : other.subtracted(*this);
    const auto differenceEraseGuard = SecureEraseGuard{difference};
    return difference.bitLength() > primeBits - 100U;
}

void Number::secureErase() noexcept {
    mem::secureErase(std::span{_words});
    _wordCount = 0U;
}

}
