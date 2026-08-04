// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ChaCha20Operations.hpp"

#include "../../../../mem/ByteIntegerAccess.hpp"
#include "../../../../mem/Endianness.hpp"
#include "../../../../mem/SecureErase.hpp"

#include <bit>
#include <span>

namespace erbsland::cryptology::impl::chacha20 {

void quarterRound(
    State &state, const std::size_t a, const std::size_t b, const std::size_t c, const std::size_t d) noexcept {
    // RFC 8439, Section 2.1.1: add, XOR, and rotate with the fixed 16/12/8/7 rotation sequence.
    state[a] += state[b];
    state[d] ^= state[a];
    state[d] = std::rotl(state[d], 16);
    state[c] += state[d];
    state[b] ^= state[c];
    state[b] = std::rotl(state[b], 12);
    state[a] += state[b];
    state[d] ^= state[a];
    state[d] = std::rotl(state[d], 8);
    state[c] += state[d];
    state[b] ^= state[c];
    state[b] = std::rotl(state[b], 7);
}

auto initialState(const mem::ConstByteSpan key, const mem::ConstByteSpan nonce, const uint32_t counter) noexcept
    -> State {
    // RFC 8439, Section 2.3: constants, eight key words, counter, and three nonce words in little-endian order.
    auto result = State{
        0x61707865U,
        0x3320646eU,
        0x79622d32U,
        0x6b206574U,
    };
    for (auto index = std::size_t{}; index < 8U; ++index) {
        result[index + 4U] = mem::getInteger<uint32_t>(key, unit::ByteIndex{index * 4U}, mem::Endianness::Little);
    }
    result[12] = counter;
    for (auto index = std::size_t{}; index < 3U; ++index) {
        result[index + 13U] = mem::getInteger<uint32_t>(nonce, unit::ByteIndex{index * 4U}, mem::Endianness::Little);
    }
    return result;
}

auto block(const mem::ConstByteSpan key, const mem::ConstByteSpan nonce, const uint32_t counter) noexcept
    -> mem::ByteArray<64> {
    auto initial = initialState(key, nonce, counter);
    auto working = initial;

    // RFC 8439, Section 2.3.1: ten column/diagonal double rounds produce twenty ChaCha rounds.
    for (auto round = std::size_t{}; round < 10U; ++round) {
        quarterRound(working, 0U, 4U, 8U, 12U);
        quarterRound(working, 1U, 5U, 9U, 13U);
        quarterRound(working, 2U, 6U, 10U, 14U);
        quarterRound(working, 3U, 7U, 11U, 15U);
        quarterRound(working, 0U, 5U, 10U, 15U);
        quarterRound(working, 1U, 6U, 11U, 12U);
        quarterRound(working, 2U, 7U, 8U, 13U);
        quarterRound(working, 3U, 4U, 9U, 14U);
    }

    // RFC 8439, Section 2.3: add the initial state and serialize every word little-endian.
    auto result = mem::ByteArray<64>{};
    for (auto index = std::size_t{}; index < working.size(); ++index) {
        working[index] += initial[index];
        result.setIntegerOrThrow(unit::ByteIndex{index * 4U}, working[index], mem::Endianness::Little);
    }
    mem::secureErase(std::span{working});
    mem::secureErase(std::span{initial});
    return result;
}

}
