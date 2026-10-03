#include "P256KeyAgreement.hpp"

#include "../algorithm/NistPrimeCurve.hpp"
#include "../SecureEraseGuard.hpp"

#include "../../../text/Literals.hpp"
#include "../../CryptologyError.hpp"

#include <cstdint>

namespace erbsland::cryptology::impl::p256_key_agreement {

using namespace mem;
using namespace text::literals;

auto isValidPrivateKey(const ConstByteSpan data) noexcept -> bool {
    if (data.size() != 32U) {
        return false;
    }
    // SEC 1 section 3.2.1: 1 <= d < n. Accumulate nonzero and subtraction borrow over all 32 bytes.
    const auto curve = NistPrimeCurve{NistPrimeCurve::Name::P256};
    const auto order = curve.numberToBigEndian(curve.order());
    auto borrow = uint64_t{0U};
    auto nonzero = uint32_t{0U};
    for (auto index = std::size_t{32U}; index-- > 0U;) {
        const auto value = data[index].toUInt8();
        nonzero |= value;
        const auto difference = uint64_t{value} - order.span()[index].toUInt8() - borrow;
        borrow = difference >> 63U;
    }
    return (borrow & static_cast<uint64_t>(nonzero != 0U)) != 0U;
}

auto isValidPublicKey(const ConstByteSpan data) noexcept -> bool {
    // RFC 8446 section 4.2.8.2 requires uncompressed form; SEC 1 section 3.2.2.1 validates the point.
    // P-256 has cofactor one, so a finite point on this curve belongs to the prime-order subgroup.
    return data.size() == 65U && data.front().toUInt8() == 4U &&
        NistPrimeCurve{NistPrimeCurve::Name::P256}.pointFromBytes(data).has_value();
}

auto publicKey(const ConstByteSpan privateKey) -> ByteBlock {
    if (!isValidPrivateKey(privateKey)) {
        throw CryptologyError{"The P-256 private scalar must be in [1,n-1]."_el};
    }
    const auto curve = NistPrimeCurve{NistPrimeCurve::Name::P256};
    auto scalar = *curve.numberFromBigEndian(privateKey);
    const auto scalarGuard = SecureEraseGuard{scalar};
    // SEC 1 section 3.2.1: Q = [d]G, using only secret-scalar arithmetic until public encoding.
    auto projective = curve.multiplyBaseSecret(scalar);
    const auto projectiveGuard = SecureEraseGuard{projective};
    auto affine = curve.toAffineSecret(projective);
    const auto affineGuard = SecureEraseGuard{affine};
    return curve.pointToUncompressed(affine);
}

auto agree(const ConstByteSpan privateKey, const ConstByteSpan peerPublicKey) -> ByteBlock {
    if (!isValidPrivateKey(privateKey) || !isValidPublicKey(peerPublicKey)) {
        throw CryptologyError{"P-256 key agreement requires a valid scalar and uncompressed peer point."_el};
    }
    const auto curve = NistPrimeCurve{NistPrimeCurve::Name::P256};
    const auto peer = *curve.pointFromBytes(peerPublicKey);
    auto scalar = *curve.numberFromBigEndian(privateKey);
    const auto scalarGuard = SecureEraseGuard{scalar};
    // SEC 1 section 3.3.1 steps 1-3: validate Q, compute P = [d]Q, and reject the point at infinity.
    auto projective = curve.multiplySecret(peer, scalar);
    const auto projectiveGuard = SecureEraseGuard{projective};
    if (curve.isIdentity(projective)) {
        throw CryptologyError{"P-256 key agreement produced the point at infinity."_el};
    }
    auto affine = curve.toAffineSecret(projective);
    const auto affineGuard = SecureEraseGuard{affine};
    // SEC 1 section 3.3.1 step 4 and RFC 8446 section 7.4.2: Z is x(P), retaining leading zero octets.
    return curve.numberToBigEndian(affine.x, true);
}

}
