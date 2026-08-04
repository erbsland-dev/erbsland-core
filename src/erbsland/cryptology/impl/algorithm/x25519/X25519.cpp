// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "X25519.hpp"

#include "FieldElement.hpp"

#include "../../../../unit/ByteIndex.hpp"
#include "../../SecureEraseGuard.hpp"

namespace erbsland::cryptology::impl::x25519 {

auto publicKey(const mem::ConstByteSpan privateKey) -> mem::ByteArray<32U> {
    // RFC 7748 sections 4.1 and 6.1: Curve25519's base-point u-coordinate is 9.
    auto basePoint = mem::ByteArray<32U>{};
    basePoint.set(unit::ByteIndex::zero(), mem::Byte{9U});
    return scalarMultiply(privateKey, basePoint.span());
}

auto agree(const mem::ConstByteSpan privateKey, const mem::ConstByteSpan peerPublicKey) -> mem::ByteArray<32U> {
    return scalarMultiply(privateKey, peerPublicKey);
}

auto scalarMultiply(const mem::ConstByteSpan scalarBytes, const mem::ConstByteSpan coordinateBytes)
    -> mem::ByteArray<32U> {
    // RFC 7748 section 5, decodeScalar25519: copy the secret scalar so clamping never modifies caller-owned bytes.
    // The guard erases the clamped scalar on success and on every exceptional exit.
    auto scalar = mem::ByteArray<32U>::fromSpanOrThrow(scalarBytes);
    const auto scalarEraseGuard = SecureEraseGuard{scalar};

    // RFC 7748 section 5: clear k[0]'s low three bits and k[31]'s high bit, then set k[31]'s second-highest bit.
    scalar.set(
        unit::ByteIndex::zero(), mem::Byte{static_cast<uint8_t>(scalar.get(unit::ByteIndex::zero()).toUInt8() & 248U)});
    scalar.set(
        unit::ByteIndex{31U},
        mem::Byte{static_cast<uint8_t>((scalar.get(unit::ByteIndex{31U}).toUInt8() & 127U) | 64U)});

    // RFC 7748 section 5: decode u as x_1, then initialize (x_2, z_2) = (1, 0), (x_3, z_3) = (u, 1), swap = 0.
    // FieldElement destructors erase every scalar-derived ladder coordinate when this operation returns.
    const auto x1 = FieldElement::fromBytes(coordinateBytes);
    auto x2 = FieldElement::one();
    auto z2 = FieldElement{};
    auto x3 = x1;
    auto z3 = FieldElement::one();
    auto swap = uint64_t{0U};

    // RFC 7748 sections 5 and 5.1: execute exactly 255 ladder iterations, from scalar bit 254 down to bit zero.
    for (auto position = 255U; position-- > 0U;) {
        const auto scalarByte = scalar.get(unit::ByteIndex::fromSizeT(position / 8U)).toUInt64();
        const auto bit = (scalarByte >> (position & 7U)) & 1U;

        // RFC 7748 section 5: swap ^= k_t; cswap(swap, x_2, x_3); cswap(swap, z_2, z_3); swap = k_t.
        swap ^= bit;
        FieldElement::conditionalSwap(x2, x3, swap);
        FieldElement::conditionalSwap(z2, z3, swap);
        swap = bit;

        // RFC 7748 section 5: translate the ladder formulas directly, retaining the specification's variable names.
        const auto a = FieldElement::add(x2, z2);
        const auto aa = FieldElement::square(a);
        const auto b = FieldElement::subtract(x2, z2);
        const auto bb = FieldElement::square(b);
        const auto e = FieldElement::subtract(aa, bb);
        const auto c = FieldElement::add(x3, z3);
        const auto d = FieldElement::subtract(x3, z3);
        const auto da = FieldElement::multiply(d, a);
        const auto cb = FieldElement::multiply(c, b);

        // RFC 7748 section 5: update x_3, z_3, x_2, and z_2; a24 = (486662 - 2) / 4 = 121665.
        x3 = FieldElement::square(FieldElement::add(da, cb));
        z3 = FieldElement::multiply(x1, FieldElement::square(FieldElement::subtract(da, cb)));
        x2 = FieldElement::multiply(aa, bb);
        z2 = FieldElement::multiply(e, FieldElement::add(aa, FieldElement::multiplySmall(e, 121665U)));
    }

    // RFC 7748 section 5: apply the pending final swap after processing k_0.
    FieldElement::conditionalSwap(x2, x3, swap);
    FieldElement::conditionalSwap(z2, z3, swap);

    // RFC 7748 section 5: convert the projective result to affine form x_2 * z_2^(p-2), then encode it.
    auto result = FieldElement::multiply(x2, FieldElement::invert(z2)).toBytes();
    // The encoded coordinate is a shared secret for agreement calls. Return a copy, then erase this stack instance.
    const auto resultEraseGuard = SecureEraseGuard{result};
    return mem::ByteArray<32U>{result};
}

}
