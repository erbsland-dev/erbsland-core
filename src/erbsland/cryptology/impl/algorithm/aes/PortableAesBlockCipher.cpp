// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PortableAesBlockCipher.hpp"

#include "AesOperations.hpp"

namespace erbsland::cryptology::impl {

PortableAesBlockCipher::~PortableAesBlockCipher() noexcept {
    secureErase();
}

auto PortableAesBlockCipher::encrypt(const Block &input) const noexcept -> Block {
    auto state = input;

    // FIPS 197, Algorithm 1: the initial AddRoundKey precedes the first full round.
    state ^= _keySchedule.roundKey(0U);
    for (auto round = std::size_t{1U}; round < _keySchedule.roundCount(); ++round) {
        substituteBytes(state);
        shiftRows(state);
        mixColumns(state);
        state ^= _keySchedule.roundKey(round);
    }
    // FIPS 197, Algorithm 1: the final round deliberately omits MixColumns.
    substituteBytes(state);
    shiftRows(state);
    state ^= _keySchedule.roundKey(_keySchedule.roundCount());
    return state;
}

auto PortableAesBlockCipher::decrypt(const Block &input) const noexcept -> Block {
    auto state = input;

    // FIPS 197, Algorithm 3: start with the last encryption round key.
    state ^= _keySchedule.roundKey(_keySchedule.roundCount());
    for (auto round = _keySchedule.roundCount() - 1U; round > 0U; --round) {
        inverseShiftRows(state);
        inverseSubstituteBytes(state);
        state ^= _keySchedule.roundKey(round);
        inverseMixColumns(state);
    }
    inverseShiftRows(state);
    inverseSubstituteBytes(state);
    state ^= _keySchedule.roundKey(0U);
    return state;
}

void PortableAesBlockCipher::secureErase() noexcept {
    _keySchedule.secureErase();
}

void PortableAesBlockCipher::substituteBytes(Block &state) noexcept {
    for (auto index = unit::ByteIndex{}; index < Block::endIndex(); ++index) {
        state.set(index, aes::substitute(state.get(index)));
    }
}

void PortableAesBlockCipher::inverseSubstituteBytes(Block &state) noexcept {
    for (auto index = unit::ByteIndex{}; index < Block::endIndex(); ++index) {
        state.set(index, aes::inverseSubstitute(state.get(index)));
    }
}

void PortableAesBlockCipher::shiftRows(Block &state) noexcept {
    const auto input = state;
    // FIPS 197, Section 3.4: state index is row + 4 * column.
    for (auto row = 0U; row < 4U; ++row) {
        for (auto column = 0U; column < 4U; ++column) {
            const auto sourceColumn = (column + row) % 4U;
            state.set(unit::ByteIndex{row + 4U * column}, input.get(unit::ByteIndex{row + 4U * sourceColumn}));
        }
    }
}

void PortableAesBlockCipher::inverseShiftRows(Block &state) noexcept {
    const auto input = state;
    for (auto row = 0U; row < 4U; ++row) {
        for (auto column = 0U; column < 4U; ++column) {
            const auto sourceColumn = (column + 4U - row) % 4U;
            state.set(unit::ByteIndex{row + 4U * column}, input.get(unit::ByteIndex{row + 4U * sourceColumn}));
        }
    }
}

void PortableAesBlockCipher::mixColumns(Block &state) noexcept {
    for (auto column = 0U; column < 4U; ++column) {
        const auto offset = column * 4U;
        const auto a0 = state.get(unit::ByteIndex{offset});
        const auto a1 = state.get(unit::ByteIndex{offset + 1U});
        const auto a2 = state.get(unit::ByteIndex{offset + 2U});
        const auto a3 = state.get(unit::ByteIndex{offset + 3U});
        // FIPS 197, Equation 5.8: multiply each column by the fixed Rijndael matrix.
        state.set(unit::ByteIndex{offset}, aes::multiply(a0, 0x02U) ^ aes::multiply(a1, 0x03U) ^ a2 ^ a3);
        state.set(unit::ByteIndex{offset + 1U}, a0 ^ aes::multiply(a1, 0x02U) ^ aes::multiply(a2, 0x03U) ^ a3);
        state.set(unit::ByteIndex{offset + 2U}, a0 ^ a1 ^ aes::multiply(a2, 0x02U) ^ aes::multiply(a3, 0x03U));
        state.set(unit::ByteIndex{offset + 3U}, aes::multiply(a0, 0x03U) ^ a1 ^ a2 ^ aes::multiply(a3, 0x02U));
    }
}

void PortableAesBlockCipher::inverseMixColumns(Block &state) noexcept {
    for (auto column = 0U; column < 4U; ++column) {
        const auto offset = column * 4U;
        const auto a0 = state.get(unit::ByteIndex{offset});
        const auto a1 = state.get(unit::ByteIndex{offset + 1U});
        const auto a2 = state.get(unit::ByteIndex{offset + 2U});
        const auto a3 = state.get(unit::ByteIndex{offset + 3U});
        // FIPS 197, Equation 5.15: multiply each column by the inverse Rijndael matrix.
        state.set(
            unit::ByteIndex{offset},
            aes::multiply(a0, 0x0eU) ^ aes::multiply(a1, 0x0bU) ^ aes::multiply(a2, 0x0dU) ^ aes::multiply(a3, 0x09U));
        state.set(
            unit::ByteIndex{offset + 1U},
            aes::multiply(a0, 0x09U) ^ aes::multiply(a1, 0x0eU) ^ aes::multiply(a2, 0x0bU) ^ aes::multiply(a3, 0x0dU));
        state.set(
            unit::ByteIndex{offset + 2U},
            aes::multiply(a0, 0x0dU) ^ aes::multiply(a1, 0x09U) ^ aes::multiply(a2, 0x0eU) ^ aes::multiply(a3, 0x0bU));
        state.set(
            unit::ByteIndex{offset + 3U},
            aes::multiply(a0, 0x0bU) ^ aes::multiply(a1, 0x0dU) ^ aes::multiply(a2, 0x09U) ^ aes::multiply(a3, 0x0eU));
    }
}

}
