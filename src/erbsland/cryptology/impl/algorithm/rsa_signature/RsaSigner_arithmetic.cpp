// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "RsaSigner.hpp"

#include "RsaMontgomeryScratch.hpp"

#include "../../SecureEraseGuard.hpp"

#include <algorithm>
#include <array>
#include <cstdint>

namespace erbsland::cryptology::impl::rsa_signer::arithmetic {

using rsa_signature::Number;

[[nodiscard]] auto select(const Number &first, const Number &second, const uint32_t mask) noexcept -> Number {
    auto result = Number{};
    result.count = first.count;
    for (auto index = std::size_t{}; index < first.count; ++index) {
        result.words[index] = (first.words[index] & ~mask) | (second.words[index] & mask);
    }
    return result;
}

[[nodiscard]] auto subtractRaw(const Number &left, const Number &right, uint32_t &borrowResult) noexcept -> Number {
    auto result = Number{};
    result.count = left.count;
    auto borrow = uint64_t{};
    for (auto index = std::size_t{}; index < left.count; ++index) {
        const auto subtrahend = uint64_t{right.words[index]} + borrow;
        const auto minuend = uint64_t{left.words[index]};
        result.words[index] = static_cast<uint32_t>(minuend - subtrahend);
        borrow = minuend < subtrahend ? 1U : 0U;
    }
    borrowResult = static_cast<uint32_t>(borrow);
    return result;
}

[[nodiscard]] auto reduceOnce(const Number &value, const Number &modulus, const uint32_t carry = 0U) noexcept
    -> Number {
    auto borrow = uint32_t{};
    const auto difference = subtractRaw(value, modulus, borrow);
    const auto useDifference = (carry | (borrow ^ 1U)) & 1U;
    return select(value, difference, uint32_t{} - useDifference);
}

[[nodiscard]] auto addModulo(const Number &left, const Number &right, const Number &modulus) noexcept -> Number {
    auto sum = Number{};
    sum.count = modulus.count;
    auto carry = uint64_t{};
    for (auto index = std::size_t{}; index < modulus.count; ++index) {
        const auto word = uint64_t{left.words[index]} + right.words[index] + carry;
        sum.words[index] = static_cast<uint32_t>(word);
        carry = word >> 32U;
    }
    return reduceOnce(sum, modulus, static_cast<uint32_t>(carry));
}

[[nodiscard]] auto subtractModulo(const Number &left, const Number &right, const Number &modulus) noexcept -> Number {
    auto borrow = uint32_t{};
    const auto difference = subtractRaw(left, right, borrow);
    auto corrected = Number{};
    corrected.count = modulus.count;
    auto carry = uint64_t{};
    for (auto index = std::size_t{}; index < modulus.count; ++index) {
        const auto word = uint64_t{difference.words[index]} + modulus.words[index] + carry;
        corrected.words[index] = static_cast<uint32_t>(word);
        carry = word >> 32U;
    }
    return select(difference, corrected, uint32_t{} - borrow);
}

[[nodiscard]] auto montgomeryFactor(const Number &modulus) noexcept -> uint32_t {
    auto inverse = uint32_t{1U};
    // HAC section 14.3.2: six Newton steps recover n[0]^-1 modulo 2^32 for every odd modulus.
    for (auto iteration = 0U; iteration < 5U; ++iteration) {
        inverse *= 2U - modulus.words[0U] * inverse;
    }
    return uint32_t{} - inverse;
}

[[nodiscard]] auto montgomeryMultiply(
    const Number &left, const Number &right, const Number &modulus, const uint32_t factor) noexcept -> Number {
    auto scratch = RsaMontgomeryScratch{};
    auto &temporary = scratch.words;
    const auto temporaryEraseGuard = SecureEraseGuard{scratch};

    // HAC section 14.3.2 algorithm 14.36: process every limb in the fixed modulus width and interleave reduction.
    for (auto rightIndex = std::size_t{}; rightIndex < modulus.count; ++rightIndex) {
        auto carry = uint64_t{};
        for (auto leftIndex = std::size_t{}; leftIndex < modulus.count; ++leftIndex) {
            const auto product =
                uint64_t{left.words[leftIndex]} * right.words[rightIndex] + temporary[leftIndex] + carry;
            temporary[leftIndex] = static_cast<uint32_t>(product);
            carry = product >> 32U;
        }
        const auto upper = uint64_t{temporary[modulus.count]} + carry;
        temporary[modulus.count] = static_cast<uint32_t>(upper);
        temporary[modulus.count + 1U] = static_cast<uint32_t>(upper >> 32U);

        const auto reduction = static_cast<uint32_t>(uint64_t{temporary[0U]} * factor);
        carry = 0U;
        for (auto index = std::size_t{}; index < modulus.count; ++index) {
            const auto product = uint64_t{reduction} * modulus.words[index] + temporary[index] + carry;
            if (index != 0U) {
                temporary[index - 1U] = static_cast<uint32_t>(product);
            }
            carry = product >> 32U;
        }
        const auto reducedUpper = uint64_t{temporary[modulus.count]} + carry;
        temporary[modulus.count - 1U] = static_cast<uint32_t>(reducedUpper);
        temporary[modulus.count] = temporary[modulus.count + 1U] + static_cast<uint32_t>(reducedUpper >> 32U);
        temporary[modulus.count + 1U] = 0U;
    }

    auto result = Number{};
    result.count = modulus.count;
    std::copy_n(temporary.begin(), modulus.count, result.words.begin());
    return reduceOnce(result, modulus, temporary[modulus.count]);
}

[[nodiscard]] auto montgomeryR2(const Number &modulus) noexcept -> Number {
    auto result = Number{};
    result.count = modulus.count;
    result.words[0U] = 1U;
    // R^2 mod n is computed with exactly 64*wordCount masked modular doublings; modulus values may be secret primes.
    for (auto iteration = std::size_t{}; iteration < modulus.count * 64U; ++iteration) {
        auto doubled = arithmetic::addModulo(result, result, modulus);
        const auto doubledEraseGuard = SecureEraseGuard{doubled};
        result.secureErase();
        result = doubled;
    }
    return result;
}

}

namespace erbsland::cryptology::impl::rsa_signer {

using rsa_signature::Number;

auto reduceSecret(const Number &value, const Number &modulus) noexcept -> Number {
    auto result = Number{};
    result.count = modulus.count;
    auto one = Number{};
    one.count = modulus.count;
    one.words[0U] = 1U;

    // RFC 8017 section 5.1.2 CRT conversion: scan the complete represented source width without value-dependent work.
    for (auto position = value.count * 32U; position-- > 0U;) {
        auto doubled = arithmetic::addModulo(result, result, modulus);
        const auto doubledEraseGuard = SecureEraseGuard{doubled};
        auto candidate = arithmetic::addModulo(doubled, one, modulus);
        const auto candidateEraseGuard = SecureEraseGuard{candidate};
        const auto bit = (value.words[position / 32U] >> (position & 31U)) & 1U;
        result.secureErase();
        result = arithmetic::select(doubled, candidate, uint32_t{} - bit);
    }
    return result;
}

auto multiplyModuloSecret(const Number &left, const Number &right, const Number &modulus) noexcept -> Number {
    auto result = Number{};
    result.count = modulus.count;
    auto addend = reduceSecret(left, modulus);
    const auto addendEraseGuard = SecureEraseGuard{addend};

    // Fixed double-and-add-always supports the even p-1 and q-1 moduli used during CRT consistency validation.
    for (auto position = std::size_t{}; position < modulus.count * 32U; ++position) {
        auto candidate = arithmetic::addModulo(result, addend, modulus);
        const auto candidateEraseGuard = SecureEraseGuard{candidate};
        const auto bit = (right.words[position / 32U] >> (position & 31U)) & 1U;
        auto selected = arithmetic::select(result, candidate, uint32_t{} - bit);
        const auto selectedEraseGuard = SecureEraseGuard{selected};
        result.secureErase();
        result = selected;
        auto doubled = arithmetic::addModulo(addend, addend, modulus);
        const auto doubledEraseGuard = SecureEraseGuard{doubled};
        addend.secureErase();
        addend = doubled;
    }
    return result;
}

auto subtractModuloSecret(const Number &left, const Number &right, const Number &modulus) noexcept -> Number {
    return arithmetic::subtractModulo(left, right, modulus);
}

auto powerModuloSecret(const Number &base, const Number &exponent, const Number &oddModulus) noexcept -> Number {
    const auto factor = arithmetic::montgomeryFactor(oddModulus);
    auto r2 = arithmetic::montgomeryR2(oddModulus);
    const auto r2EraseGuard = SecureEraseGuard{r2};
    auto one = Number{};
    one.count = oddModulus.count;
    one.words[0U] = 1U;
    auto result = arithmetic::montgomeryMultiply(one, r2, oddModulus, factor);
    const auto resultEraseGuard = SecureEraseGuard{result};
    auto reducedBase = reduceSecret(base, oddModulus);
    const auto reducedBaseEraseGuard = SecureEraseGuard{reducedBase};
    auto factorValue = arithmetic::montgomeryMultiply(reducedBase, r2, oddModulus, factor);
    const auto factorEraseGuard = SecureEraseGuard{factorValue};

    // RFC 8017 section 5.1.2 exponentiation: square and multiply for every bit, then select without branching.
    for (auto position = oddModulus.count * 32U; position-- > 0U;) {
        auto squared = arithmetic::montgomeryMultiply(result, result, oddModulus, factor);
        const auto squaredEraseGuard = SecureEraseGuard{squared};
        auto multiplied = arithmetic::montgomeryMultiply(squared, factorValue, oddModulus, factor);
        const auto multipliedEraseGuard = SecureEraseGuard{multiplied};
        const auto bit = (exponent.words[position / 32U] >> (position & 31U)) & 1U;
        result.secureErase();
        result = arithmetic::select(squared, multiplied, uint32_t{} - bit);
    }
    return arithmetic::montgomeryMultiply(result, one, oddModulus, factor);
}

auto multiplyExact(const Number &left, const Number &right, const std::size_t resultWords) noexcept -> Number {
    auto result = Number{};
    result.count = resultWords;
    // RFC 8017 appendix A.1.2 consistency n=pq: fixed schoolbook multiplication preserves the complete product.
    for (auto leftIndex = std::size_t{}; leftIndex < left.count; ++leftIndex) {
        auto carry = uint64_t{};
        for (auto rightIndex = std::size_t{}; rightIndex < right.count; ++rightIndex) {
            const auto resultIndex = leftIndex + rightIndex;
            if (resultIndex >= resultWords) {
                continue;
            }
            const auto word =
                uint64_t{left.words[leftIndex]} * right.words[rightIndex] + result.words[resultIndex] + carry;
            result.words[resultIndex] = static_cast<uint32_t>(word);
            carry = word >> 32U;
        }
        if (leftIndex + right.count < resultWords) {
            result.words[leftIndex + right.count] = static_cast<uint32_t>(carry);
        }
    }
    return result;
}

}
