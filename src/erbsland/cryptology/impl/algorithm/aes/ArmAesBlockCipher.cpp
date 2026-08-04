// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ArmAesBlockCipher.hpp"

#include "../../../../mem/impl/UnsafeByteArrayAccess.hpp"

#include <arm_neon.h>

namespace erbsland::cryptology::impl {

ArmAesBlockCipher::~ArmAesBlockCipher() noexcept {
    secureErase();
}

auto ArmAesBlockCipher::encrypt(const Block &input) const noexcept -> Block {
    auto state = vld1q_u8(reinterpret_cast<const uint8_t *>(input.span().data()));
    auto roundKey = Block{};
    // Arm AESE performs AddRoundKey, SubBytes, and ShiftRows; AESMC performs MixColumns.
    for (auto round = std::size_t{}; round < _keySchedule.roundCount() - 1U; ++round) {
        roundKey = _keySchedule.roundKey(round);
        state = vaeseq_u8(state, vld1q_u8(reinterpret_cast<const uint8_t *>(roundKey.span().data())));
        state = vaesmcq_u8(state);
    }
    roundKey = _keySchedule.roundKey(_keySchedule.roundCount() - 1U);
    state = vaeseq_u8(state, vld1q_u8(reinterpret_cast<const uint8_t *>(roundKey.span().data())));
    roundKey = _keySchedule.roundKey(_keySchedule.roundCount());
    state = veorq_u8(state, vld1q_u8(reinterpret_cast<const uint8_t *>(roundKey.span().data())));

    auto result = Block{};
    auto output = mem::impl::UnsafeByteArrayAccess{result}.writableData();
    vst1q_u8(reinterpret_cast<uint8_t *>(output.data()), state);
    roundKey.secureErase();
    return result;
}

auto ArmAesBlockCipher::decrypt(const Block &input) const noexcept -> Block {
    auto state = vld1q_u8(reinterpret_cast<const uint8_t *>(input.span().data()));
    const auto zero = vdupq_n_u8(0U);
    auto roundKey = _keySchedule.roundKey(_keySchedule.roundCount());
    state = veorq_u8(state, vld1q_u8(reinterpret_cast<const uint8_t *>(roundKey.span().data())));
    // FIPS 197, Algorithm 3: use AESD with a zero key for InvShiftRows/InvSubBytes, then add the raw round key.
    for (auto round = _keySchedule.roundCount() - 1U; round > 0U; --round) {
        state = vaesdq_u8(state, zero);
        roundKey = _keySchedule.roundKey(round);
        state = veorq_u8(state, vld1q_u8(reinterpret_cast<const uint8_t *>(roundKey.span().data())));
        state = vaesimcq_u8(state);
    }
    state = vaesdq_u8(state, zero);
    roundKey = _keySchedule.roundKey(0U);
    state = veorq_u8(state, vld1q_u8(reinterpret_cast<const uint8_t *>(roundKey.span().data())));

    auto result = Block{};
    auto output = mem::impl::UnsafeByteArrayAccess{result}.writableData();
    vst1q_u8(reinterpret_cast<uint8_t *>(output.data()), state);
    roundKey.secureErase();
    return result;
}

void ArmAesBlockCipher::secureErase() noexcept {
    _keySchedule.secureErase();
}

}
