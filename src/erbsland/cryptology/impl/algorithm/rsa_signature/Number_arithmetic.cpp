// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Number.hpp"

#include "RsaMontgomeryScratch.hpp"

#include "../../SecureEraseGuard.hpp"

#include <algorithm>

namespace erbsland::cryptology::impl::rsa_signature {

auto Number::select(const Number &first, const Number &second, const uint32_t mask) noexcept -> Number {
    auto result = Number{};
    result._wordCount = first._wordCount;
    for (auto index = std::size_t{}; index < first._wordCount; ++index) {
        result._words[index] = (first._words[index] & ~mask) | (second._words[index] & mask);
    }
    return result;
}

auto Number::subtractRaw(const Number &left, const Number &right, uint32_t &borrowResult) noexcept -> Number {
    auto result = Number{};
    result._wordCount = left._wordCount;
    auto borrow = uint64_t{};
    for (auto index = std::size_t{}; index < left._wordCount; ++index) {
        const auto subtrahend = uint64_t{right._words[index]} + borrow;
        const auto minuend = uint64_t{left._words[index]};
        result._words[index] = static_cast<uint32_t>(minuend - subtrahend);
        borrow = minuend < subtrahend ? 1U : 0U;
    }
    borrowResult = static_cast<uint32_t>(borrow);
    return result;
}

auto Number::reducedOnce(const Number &modulus, const uint32_t carry) const noexcept -> Number {
    auto borrow = uint32_t{};
    auto difference = subtractRaw(*this, modulus, borrow);
    const auto differenceEraseGuard = SecureEraseGuard{difference};
    const auto useDifference = (carry | (borrow ^ 1U)) & 1U;
    return select(*this, difference, uint32_t{} - useDifference);
}

auto Number::addedModuloSecret(const Number &other, const Number &modulus) const noexcept -> Number {
    auto sum = Number{};
    const auto sumEraseGuard = SecureEraseGuard{sum};
    sum._wordCount = modulus._wordCount;
    auto carry = uint64_t{};
    for (auto index = std::size_t{}; index < modulus._wordCount; ++index) {
        const auto value = uint64_t{_words[index]} + other._words[index] + carry;
        sum._words[index] = static_cast<uint32_t>(value);
        carry = value >> cWordBits;
    }
    return sum.reducedOnce(modulus, static_cast<uint32_t>(carry));
}

auto Number::montgomeryFactor() const noexcept -> uint32_t {
    auto inverse = uint32_t{1U};
    for (auto iteration = 0U; iteration < 5U; ++iteration) {
        inverse *= 2U - _words[0U] * inverse;
    }
    return uint32_t{} - inverse;
}

auto Number::montgomeryMultiply(
    const Number &left, const Number &right, const Number &modulus, const uint32_t factor) noexcept -> Number {
    auto scratch = rsa_signer::arithmetic::RsaMontgomeryScratch{};
    auto &temporary = scratch.words;
    const auto temporaryEraseGuard = SecureEraseGuard{scratch};
    for (auto rightIndex = std::size_t{}; rightIndex < modulus._wordCount; ++rightIndex) {
        auto carry = uint64_t{};
        for (auto leftIndex = std::size_t{}; leftIndex < modulus._wordCount; ++leftIndex) {
            const auto product =
                uint64_t{left._words[leftIndex]} * right._words[rightIndex] + temporary[leftIndex] + carry;
            temporary[leftIndex] = static_cast<uint32_t>(product);
            carry = product >> cWordBits;
        }
        const auto upper = uint64_t{temporary[modulus._wordCount]} + carry;
        temporary[modulus._wordCount] = static_cast<uint32_t>(upper);
        temporary[modulus._wordCount + 1U] = static_cast<uint32_t>(upper >> cWordBits);
        const auto reduction = static_cast<uint32_t>(uint64_t{temporary[0U]} * factor);
        carry = 0U;
        for (auto index = std::size_t{}; index < modulus._wordCount; ++index) {
            const auto product = uint64_t{reduction} * modulus._words[index] + temporary[index] + carry;
            if (index != 0U) {
                temporary[index - 1U] = static_cast<uint32_t>(product);
            }
            carry = product >> cWordBits;
        }
        const auto reducedUpper = uint64_t{temporary[modulus._wordCount]} + carry;
        temporary[modulus._wordCount - 1U] = static_cast<uint32_t>(reducedUpper);
        temporary[modulus._wordCount] =
            temporary[modulus._wordCount + 1U] + static_cast<uint32_t>(reducedUpper >> cWordBits);
        temporary[modulus._wordCount + 1U] = 0U;
    }
    auto result = Number{};
    const auto resultEraseGuard = SecureEraseGuard{result};
    result._wordCount = modulus._wordCount;
    std::copy_n(temporary.begin(), modulus._wordCount, result._words.begin());
    return result.reducedOnce(modulus, temporary[modulus._wordCount]);
}

auto Number::montgomeryR2() const noexcept -> Number {
    auto result = fromValue(1U, _wordCount);
    for (auto iteration = std::size_t{}; iteration < _wordCount * 64U; ++iteration) {
        auto doubled = result.addedModuloSecret(result, *this);
        const auto doubledEraseGuard = SecureEraseGuard{doubled};
        result.secureErase();
        result = doubled;
    }
    return result;
}

auto Number::reducedSecret(const Number &modulus) const noexcept -> Number {
    auto result = fromValue(0U, modulus._wordCount);
    const auto one = fromValue(1U, modulus._wordCount);
    for (auto position = _wordCount * cWordBits; position-- > 0U;) {
        auto doubled = result.addedModuloSecret(result, modulus);
        const auto doubledEraseGuard = SecureEraseGuard{doubled};
        auto candidate = doubled.addedModuloSecret(one, modulus);
        const auto candidateEraseGuard = SecureEraseGuard{candidate};
        const auto bit = (_words[position / cWordBits] >> (position & (cWordBits - 1U))) & 1U;
        result.secureErase();
        result = select(doubled, candidate, uint32_t{} - bit);
    }
    return result;
}

auto Number::multipliedModuloSecret(const Number &other, const Number &modulus) const noexcept -> Number {
    auto result = fromValue(0U, modulus._wordCount);
    auto addend = reducedSecret(modulus);
    const auto addendEraseGuard = SecureEraseGuard{addend};
    for (auto position = std::size_t{}; position < modulus._wordCount * cWordBits; ++position) {
        auto candidate = result.addedModuloSecret(addend, modulus);
        const auto candidateEraseGuard = SecureEraseGuard{candidate};
        const auto bit = (other._words[position / cWordBits] >> (position & (cWordBits - 1U))) & 1U;
        auto selected = select(result, candidate, uint32_t{} - bit);
        const auto selectedEraseGuard = SecureEraseGuard{selected};
        result.secureErase();
        result = selected;
        auto doubled = addend.addedModuloSecret(addend, modulus);
        const auto doubledEraseGuard = SecureEraseGuard{doubled};
        addend.secureErase();
        addend = doubled;
    }
    return result;
}

auto Number::subtractedModuloSecret(const Number &other, const Number &modulus) const noexcept -> Number {
    auto borrow = uint32_t{};
    auto difference = subtractRaw(*this, other, borrow);
    const auto differenceEraseGuard = SecureEraseGuard{difference};
    auto corrected = difference.added(modulus, modulus._wordCount);
    const auto correctedEraseGuard = SecureEraseGuard{corrected};
    return select(difference, corrected, uint32_t{} - borrow);
}

auto Number::poweredModuloSecret(const Number &exponent, const Number &oddModulus) const noexcept -> Number {
    const auto factor = oddModulus.montgomeryFactor();
    auto r2 = oddModulus.montgomeryR2();
    const auto r2EraseGuard = SecureEraseGuard{r2};
    const auto one = fromValue(1U, oddModulus._wordCount);
    auto result = montgomeryMultiply(one, r2, oddModulus, factor);
    const auto resultEraseGuard = SecureEraseGuard{result};
    auto reducedBase = reducedSecret(oddModulus);
    const auto reducedBaseEraseGuard = SecureEraseGuard{reducedBase};
    auto factorValue = montgomeryMultiply(reducedBase, r2, oddModulus, factor);
    const auto factorEraseGuard = SecureEraseGuard{factorValue};
    for (auto position = oddModulus._wordCount * cWordBits; position-- > 0U;) {
        auto squared = montgomeryMultiply(result, result, oddModulus, factor);
        const auto squaredEraseGuard = SecureEraseGuard{squared};
        auto multiplied = montgomeryMultiply(squared, factorValue, oddModulus, factor);
        const auto multipliedEraseGuard = SecureEraseGuard{multiplied};
        const auto bit = (exponent._words[position / cWordBits] >> (position & (cWordBits - 1U))) & 1U;
        result.secureErase();
        result = select(squared, multiplied, uint32_t{} - bit);
    }
    return montgomeryMultiply(result, one, oddModulus, factor);
}

}
