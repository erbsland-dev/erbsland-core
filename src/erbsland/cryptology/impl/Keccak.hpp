// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>

namespace erbsland::cryptology::impl {

/// Rotate the bits in a 64bit value.
[[nodiscard]] constexpr auto rotl64(const uint64_t value, const int shift) noexcept -> uint64_t {
    return std::rotl(value, shift);
}

/// The rounding constants for the Keccak F1600 permutation function.
/// Source: https://keccak.team/keccak_specs_summary.html
constexpr std::array<uint64_t, 24> cKeccakF1600RoundConstants = {
    0x0000000000000001ULL,
    0x0000000000008082ULL,
    0x800000000000808aULL,
    0x8000000080008000ULL,
    0x000000000000808bULL,
    0x0000000080000001ULL,
    0x8000000080008081ULL,
    0x8000000000008009ULL,
    0x000000000000008aULL,
    0x0000000000000088ULL,
    0x0000000080008009ULL,
    0x000000008000000aULL,
    0x000000008000808bULL,
    0x800000000000008bULL,
    0x8000000000008089ULL,
    0x8000000000008003ULL,
    0x8000000000008002ULL,
    0x8000000000000080ULL,
    0x000000000000800aULL,
    0x800000008000000aULL,
    0x8000000080008081ULL,
    0x8000000000008080ULL,
    0x0000000080000001ULL,
    0x8000000080008008ULL};

/// The rotation offsets of the Keccak F1600 permutation function.
/// Source: https://keccak.team/keccak_specs_summary.html
constexpr std::array<std::array<int, 5>, 5> cKeccakF1600RotationOffsets{
    {{0, 36, 3, 41, 18}, {1, 44, 10, 45, 2}, {62, 6, 43, 15, 61}, {28, 55, 25, 21, 56}, {27, 20, 39, 8, 14}}};

/// The state block of the Keccak F1600 permutation function.
using KeccakF1600State = std::array<uint64_t, 25>;

/// The \c theta step of the permutation.
inline void keccakTheta(KeccakF1600State &state) {
    std::array<uint64_t, 5> c{};
    std::array<uint64_t, 5> d{};

    for (std::size_t x = 0; x < 5; ++x) {
        c[x] = state[x] ^ state[x + 5] ^ state[x + 10] ^ state[x + 15] ^ state[x + 20];
    }
    for (std::size_t x = 0; x < 5; ++x) {
        d[x] = c[(x + 4) % 5] ^ rotl64(c[(x + 1) % 5], 1);
    }
    for (std::size_t y = 0; y < 5; ++y) {
        for (std::size_t x = 0; x < 5; ++x) {
            state[x + 5 * y] ^= d[x];
        }
    }
}

/// The \c rho and \c pi steps combined.
inline void keccakRhoPi(const KeccakF1600State &inState, KeccakF1600State &outState) {
    for (std::size_t y = 0; y < 5; ++y) {
        for (std::size_t x = 0; x < 5; ++x) {
            outState[y + 5 * ((2 * x + 3 * y) % 5)] = rotl64(inState[x + 5 * y], cKeccakF1600RotationOffsets[x][y]);
        }
    }
}

/// The \c chi step of the permutation.
inline void keccakChi(KeccakF1600State &state, const KeccakF1600State &b) {
    for (std::size_t x = 0; x < 5; ++x) {
        for (std::size_t y = 0; y < 5; ++y) {
            state[x + 5 * y] = b[x + 5 * y] ^ ((~b[((x + 1) % 5) + 5 * y]) & b[((x + 2) % 5) + 5 * y]);
        }
    }
}

/// Apply the round constant (\c iota step).
inline void keccakIota(KeccakF1600State &state, std::size_t round) {
    state[0] ^= cKeccakF1600RoundConstants[round];
}

/// Apply the Keccak F1600 permutation to the state.
inline void keccakF1600Permutation(KeccakF1600State &state) {
    for (std::size_t round = 0; round < 24; ++round) {
        keccakTheta(state);
        KeccakF1600State b{};
        keccakRhoPi(state, b);
        keccakChi(state, b);
        keccakIota(state, round);
    }
}

}
