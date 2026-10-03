// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "DeterministicSecureRandom.hpp"
#include "EcSigningKeyTestHelper.hpp"

#include <erbsland/cryptology/impl/EcPrivateScalarPolicy.hpp>

#include <string>
#include <string_view>

namespace ec_private_scalar_policy = el::cryptology::impl::ec_private_scalar_policy;

TESTED_TARGETS(EcPrivateScalarPolicy)
class EcPrivateScalarPolicyTest final : public UNITTEST_SUBCLASS(EcSigningKeyTestHelper) {
public:
    void testP256Boundaries() {
        const auto name = CurveName::P256;
        requireAcceptance(name, "0000000000000000000000000000000000000000000000000000000000000000", false);
        requireAcceptance(name, "0000000000000000000000000000000000000000000000000000000000000001", false);
        requireAcceptance(name, "0000000000000000000000000000000000000000000000000000000000000004", false);
        requireAcceptance(name, "00000000000000000000000000000000000000000000000000000000000000ff", false);
        requireAcceptance(name, "0000000000000000ffffffffffffffffffffffffffffffffffffffffffffffff", false);
        requireAcceptance(name, "0000000000000001000000000000000000000000000000000000000000000000", true);
        requireAcceptance(name, "0000000000000001000000000000000000000000000000000000000000000001", true);
        requireAcceptance(name, "fffffffeffffffffffffffffffffffffbce6faada7179e84f3b9cac2fc632550", true);
        requireAcceptance(name, "fffffffeffffffffffffffffffffffffbce6faada7179e84f3b9cac2fc632551", true);
        requireAcceptance(name, "fffffffeffffffffffffffffffffffffbce6faada7179e84f3b9cac2fc632552", false);
        requireAcceptance(name, "ffffffff00000000ffffffffffffffffbce6faada7179e84f3b9cac2fc632550", false);
        requireAcceptance(name, "ffffffff00000000ffffffffffffffffbce6faada7179e84f3b9cac2fc632551", false);
        requireAcceptance(name, "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", false);
    }
    void testP384Boundaries() {
        const auto name = CurveName::P384;
        requireAcceptance(
            name,
            "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
            false);
        requireAcceptance(
            name,
            "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001",
            false);
        requireAcceptance(
            name,
            "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000004",
            false);
        requireAcceptance(
            name,
            "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000ff",
            false);
        requireAcceptance(
            name,
            "0000000000000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
            false);
        requireAcceptance(
            name,
            "000000000000000100000000000000000000000000000000000000000000000000000000000000000000000000000000",
            true);
        requireAcceptance(
            name,
            "000000000000000100000000000000000000000000000000000000000000000000000000000000000000000000000001",
            true);
        requireAcceptance(
            name,
            "fffffffffffffffeffffffffffffffffffffffffffffffffc7634d81f4372ddf581a0db248b0a77aecec196accc52972",
            true);
        requireAcceptance(
            name,
            "fffffffffffffffeffffffffffffffffffffffffffffffffc7634d81f4372ddf581a0db248b0a77aecec196accc52973",
            true);
        requireAcceptance(
            name,
            "fffffffffffffffeffffffffffffffffffffffffffffffffc7634d81f4372ddf581a0db248b0a77aecec196accc52974",
            false);
        requireAcceptance(
            name,
            "ffffffffffffffffffffffffffffffffffffffffffffffffc7634d81f4372ddf581a0db248b0a77aecec196accc52972",
            false);
        requireAcceptance(
            name,
            "ffffffffffffffffffffffffffffffffffffffffffffffffc7634d81f4372ddf581a0db248b0a77aecec196accc52973",
            false);
        requireAcceptance(
            name,
            "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
            false);
    }

    void testExactWidths() {
        for (const auto name : cCurves) {
            const auto accepted = acceptedScalar(name);
            REQUIRE(ec_private_scalar_policy::isAcceptable(accepted.span(), name));
            REQUIRE_FALSE(ec_private_scalar_policy::isAcceptable({}, name));
            REQUIRE_FALSE(ec_private_scalar_policy::isAcceptable(accepted.span().subspan(1U), name));
            auto oversized = el::ByteBlockEditor{};
            oversized.append(el::Byte{}).append(accepted);
            REQUIRE_FALSE(ec_private_scalar_policy::isAcceptable(oversized.span(), name));
        }
    }

    void testConsecutiveByteRandomnessRemainsAccepted() {
        for (const auto name : cCurves) {
            const auto curve = el::cryptology::impl::NistPrimeCurve{name};
            // Exercise every starting byte, including zero, without treating deterministic bytes as secure entropy.
            for (auto sample = 0U; sample < 256U; ++sample) {
                auto fresh = el::test::DeterministicSecureRandom{};
                (void)fresh.buildByteBlock(el::ByteCount::fromInteger(sample));
                const auto bytes = fresh.buildByteBlock(curve.byteLength());
                REQUIRE(ec_private_scalar_policy::isAcceptable(bytes.span(), name));
            }
        }
    }

private:
    void requireAcceptance(const CurveName name, const std::string_view hex, const bool expected) {
        runWithContext(
            SOURCE_LOCATION(),
            [&]() -> void {
                REQUIRE_EQUAL(ec_private_scalar_policy::isAcceptable(bytesFromHex(hex).span(), name), expected);
            },
            [&]() -> std::string { return std::string{hex}; });
    }
};
