// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CryptologyResponseReader.hpp"
#include "CryptologyTestHelper.hpp"

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/core/Application.hpp>
#include <erbsland/cryptology/configuration/CryptologyConfiguration.hpp>
#include <erbsland/cryptology/CryptologyError.hpp>
#include <erbsland/cryptology/impl/algorithm/x25519/X25519.hpp>
#include <erbsland/cryptology/keys/KeyAgreementAlgorithm.hpp>
#include <erbsland/cryptology/keys/KeyAgreementPrivateKey.hpp>
#include <erbsland/cryptology/keys/KeyAgreementPublicKey.hpp>
#include <erbsland/cryptology/protected_data/ProtectedDataMode.hpp>
#include <erbsland/text/Literals.hpp>

using namespace el::cryptology;
using namespace el::text::literals;

TESTED_TARGETS(X25519 KeyAgreementPrivateKey)
class X25519FullTest final : public UNITTEST_SUBCLASS(CryptologyTestHelper) {
public:
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testWycheproof() {
        const auto applicationScope = ApplicationTestScope<>{};
        el::core::application().cryptologyConfiguration().setProtectedDataMode(ProtectedDataMode::InternalOnly);
        const auto algorithm = KeyAgreementAlgorithm{KeyAgreementAlgorithm::X25519};
        const auto records = CryptologyResponseReader{"data/cryptology/x25519/wycheproof.rsp"_el}.read();
        REQUIRE_EQUAL(records.size(), std::size_t{518U});
        for (const auto &record : records) {
            runWithContext(
                SOURCE_LOCATION(),
                [&]() -> void {
                    record.requireAllowedSettings({"Curve"_el});
                    record.requireAllowedValues(
                        {"Count"_el, "Result"_el, "Flags"_el, "Public"_el, "Private"_el, "Shared"_el});
                    REQUIRE_EQUAL(record.setting("Curve"_el), "curve25519"_el);
                    const auto publicData = bytesFromHex(record.value("Public"_el));
                    const auto privateData = bytesFromHex(record.value("Private"_el));
                    const auto expected = bytesFromHex(record.value("Shared"_el));
                    const auto actual = el::cryptology::impl::x25519::agree(privateData.span(), publicData.span());
                    REQUIRE_EQUAL(el::ByteBlock{actual}, expected);
                    if (record.value("Flags"_el).contains("ZeroSharedSecret"_el)) {
                        const auto privateKey = KeyAgreementPrivateKey::fromBytes(algorithm, privateData.span());
                        const auto publicKey = KeyAgreementPublicKey{algorithm, publicData.span()};
                        REQUIRE_THROWS_AS(CryptologyError, privateKey.agree(publicKey));
                    }
                },
                [&]() -> std::string { return el::StringConverter{record.diagnostic()}.toStdString(); });
        }
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testRfc7748OneThousandIterations() {
        auto scalar = bytesFromHex("0900000000000000000000000000000000000000000000000000000000000000");
        auto coordinate = scalar;
        for (auto iteration = std::size_t{0}; iteration < 1000U; ++iteration) {
            const auto result = el::cryptology::impl::x25519::agree(scalar.span(), coordinate.span());
            coordinate = scalar;
            scalar = el::ByteBlock{result};
        }
        REQUIRE_EQUAL(scalar, bytesFromHex("684cf59ba83309552800ef566f2f4d3c1c3887c49360e3875f2eb94d99532c51"));
    }
};
