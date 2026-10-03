// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/CryptologyError.hpp>
#include <erbsland/HttpStatus.hpp>
#include <erbsland/KeyAgreementPrivateKey.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/NotFoundError.hpp>
#include <erbsland/OutOfRangeError.hpp>
#include <erbsland/ParameterFailure.hpp>
#include <erbsland/re/impl/diagnostics/OperationData.hpp>
#include <erbsland/re/impl/text/TextAnchor.hpp>
#include <erbsland/SigningPrivateKey.hpp>
#include <erbsland/TlsTrafficSecret.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::text::literals;

TESTED_TARGETS(HttpStatus SigningPrivateKey KeyAgreementPrivateKey TlsTrafficSecret)
class FactoryErrorContractTest final : public el::UnitTest {
public:
    void testHttpStatusDataBounds() {
        auto applicationScope = ApplicationTestScope<>{};
        REQUIRE_THROWS_AS(el::err::OutOfRangeError, el::network::HttpStatus::fromCodeOrThrow(99U));
        REQUIRE_THROWS_AS(el::err::OutOfRangeError, el::network::HttpStatus::fromCodeOrThrow(600U));
        REQUIRE_EQUAL(el::network::HttpStatus::fromCodeOrThrow(200U), el::network::HttpStatus::fromCode(200U));
    }

    void testUnknownNamesAreRecoverable() {
        auto applicationScope = ApplicationTestScope<>{};
        REQUIRE_THROWS_AS(el::err::NotFoundError, el::re::impl::TextAnchor::fromString("unknown"_el));
        REQUIRE_THROWS_AS(el::err::NotFoundError, el::re::impl::baseOperationForString("unknown"_el));
    }

    void testKeyImportLengthIsData() {
        auto applicationScope = ApplicationTestScope<>{};
        REQUIRE_THROWS_AS(
            el::err::OutOfRangeError,
            el::cryptology::KeyAgreementPrivateKey::fromBytes(el::cryptology::KeyAgreementAlgorithm::X25519, {}));
        REQUIRE_THROWS_AS(
            el::err::OutOfRangeError,
            el::cryptology::TlsTrafficSecret::fromBytes(
                el::cryptology::HashAlgorithm::Sha2_256, el::mem::ConstByteSpan{}));
    }

    void testEmptyImportPasswordUsesTheRuntimeFallback() {
        auto applicationScope = ApplicationTestScope<>{};
        const auto encoded = el::mem::ByteBlock{};
        REQUIRE_THROWS_AS(
            el::cryptology::CryptologyError, el::cryptology::SigningPrivateKey::fromEncryptedDerOrThrow(encoded, {}));
        REQUIRE(el::cryptology::SigningPrivateKey::fromEncryptedDer(encoded, {}).isEmpty());
        REQUIRE(el::cryptology::SigningPrivateKey::fromEncryptedPem({}, {}).isEmpty());
    }

    void testInvalidAlgorithmRemainsAPrecondition() {
        auto applicationScope = ApplicationTestScope<>{};
        REQUIRE_THROWS_AS(
            el::err::ParameterFailure,
            el::cryptology::TlsTrafficSecret::fromBytes(el::cryptology::HashAlgorithm{}, el::mem::ConstByteSpan{}));
    }
};
