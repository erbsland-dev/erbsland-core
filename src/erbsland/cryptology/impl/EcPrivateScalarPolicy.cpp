// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EcPrivateScalarPolicy.hpp"

#include <cstddef>
#include <cstdint>

namespace erbsland::cryptology::impl::ec_private_scalar_policy {

auto isAcceptable(const mem::ConstByteSpan bytes, const NistPrimeCurve::Name curveName) noexcept -> bool {
    const auto curve = NistPrimeCurve{curveName};
    if (unit::ByteCount::fromSizeT(bytes.size()) != curve.byteLength()) {
        return false;
    }
    const auto scalar = *curve.numberFromBigEndian(bytes);
    const auto order = curve.order();
    auto distance = NistPrimeCurve::Number{};
    auto borrow = uint64_t{};
    const auto wordCount = curve.byteLength().toSizeT() / sizeof(uint32_t);
    for (auto index = std::size_t{}; index < wordCount; ++index) {
        const auto difference = uint64_t{order.words()[index]} - scalar.words()[index] - borrow;
        distance.words()[index] = static_cast<uint32_t>(difference);
        borrow = difference >> 63U;
    }
    // A nonzero upper 64-bit portion means the value is at least 2^(b-64). Evaluate both ends without
    // secret-dependent early exits; the final borrow also rejects values greater than the group order.
    const auto scalarHigh = scalar.words()[wordCount - 1U] | scalar.words()[wordCount - 2U];
    const auto distanceHigh = distance.words()[wordCount - 1U] | distance.words()[wordCount - 2U];
    return (borrow == 0U) & (scalarHigh != 0U) & (distanceHigh != 0U);
}

}
