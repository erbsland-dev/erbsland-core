// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "X86AesBlockCipher.hpp"

#include "../../../../mem/impl/UnsafeByteArrayAccess.hpp"

#include <wmmintrin.h>

namespace erbsland::cryptology::impl {

X86AesBlockCipher::~X86AesBlockCipher() noexcept {
    secureErase();
}

auto X86AesBlockCipher::encrypt(const Block &input) const noexcept -> Block {
    auto state = _mm_loadu_si128(reinterpret_cast<const __m128i *>(input.span().data()));
    auto roundKey = _keySchedule.roundKey(0U);
    state = _mm_xor_si128(state, _mm_loadu_si128(reinterpret_cast<const __m128i *>(roundKey.span().data())));
    // AESENC performs SubBytes, ShiftRows, MixColumns, and AddRoundKey for each full FIPS 197 round.
    for (auto round = std::size_t{1U}; round < _keySchedule.roundCount(); ++round) {
        roundKey = _keySchedule.roundKey(round);
        state = _mm_aesenc_si128(state, _mm_loadu_si128(reinterpret_cast<const __m128i *>(roundKey.span().data())));
    }
    roundKey = _keySchedule.roundKey(_keySchedule.roundCount());
    state = _mm_aesenclast_si128(state, _mm_loadu_si128(reinterpret_cast<const __m128i *>(roundKey.span().data())));

    auto result = Block{};
    auto output = mem::impl::UnsafeByteArrayAccess{result}.writableData();
    _mm_storeu_si128(reinterpret_cast<__m128i *>(output.data()), state);
    roundKey.secureErase();
    return result;
}

auto X86AesBlockCipher::decrypt(const Block &input) const noexcept -> Block {
    auto state = _mm_loadu_si128(reinterpret_cast<const __m128i *>(input.span().data()));
    auto roundKey = _keySchedule.roundKey(_keySchedule.roundCount());
    state = _mm_xor_si128(state, _mm_loadu_si128(reinterpret_cast<const __m128i *>(roundKey.span().data())));
    // AESDEC consumes inverse-mixed intermediate round keys for the equivalent inverse cipher.
    for (auto round = _keySchedule.roundCount() - 1U; round > 0U; --round) {
        roundKey = _keySchedule.roundKey(round);
        auto nativeKey = _mm_loadu_si128(reinterpret_cast<const __m128i *>(roundKey.span().data()));
        nativeKey = _mm_aesimc_si128(nativeKey);
        state = _mm_aesdec_si128(state, nativeKey);
    }
    roundKey = _keySchedule.roundKey(0U);
    state = _mm_aesdeclast_si128(state, _mm_loadu_si128(reinterpret_cast<const __m128i *>(roundKey.span().data())));

    auto result = Block{};
    auto output = mem::impl::UnsafeByteArrayAccess{result}.writableData();
    _mm_storeu_si128(reinterpret_cast<__m128i *>(output.data()), state);
    roundKey.secureErase();
    return result;
}

void X86AesBlockCipher::secureErase() noexcept {
    _keySchedule.secureErase();
}

}
