// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "X509TestData.hpp"

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/Application.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/ParameterFailure.hpp>
#include <erbsland/ParseError.hpp>
#include <erbsland/Path.hpp>
#include <erbsland/PathContent.hpp>
#include <erbsland/PathOperations.hpp>
#include <erbsland/TempDirectory.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <erbsland/X509Certificate.hpp>
#include <erbsland/X509CertificateBundle.hpp>

#include <cstdint>
#include <vector>

using namespace el::cryptology;
using namespace el::text::literals;
using erbsland::test::x509::certificatePem;

TESTED_TARGETS(X509Certificate X509CertificateBundle PemDerFormat)
class X509CertificateFileTest final : public el::UnitTest {
public:
    void testAutomaticReadAndWrite() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto directory = el::path::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
        const auto certificate = X509Certificate::fromPemOrThrow(certificatePem());
        const auto pemPath = directory->path() / "certificate.pem"_el;
        const auto derPath = directory->path() / "certificate.der"_el;
        const auto crtPath = directory->path() / "certificate.crt"_el;
        const auto cerPath = directory->path() / "certificate.cer"_el;
        certificate.writeToFile(pemPath);
        certificate.writeToFile(derPath);
        certificate.writeToFile(crtPath);
        certificate.writeToFile(cerPath);
        REQUIRE_EQUAL(X509Certificate::fromFileOrThrow(pemPath).toDer(), certificate.toDer());
        REQUIRE_EQUAL(X509Certificate::fromFileOrThrow(derPath).toDer(), certificate.toDer());
        REQUIRE_EQUAL(X509Certificate::fromFileOrThrow(crtPath).toDer(), certificate.toDer());
        REQUIRE_EQUAL(X509Certificate::fromFileOrThrow(cerPath).toDer(), certificate.toDer());
        REQUIRE(crtPath.content().readTextOrThrow().startsWith("-----BEGIN CERTIFICATE-----"_el));
        REQUIRE_FALSE(cerPath.content().readDataOrThrow().startsWith(
            el::mem::ByteBlock::fromVector(std::vector<uint8_t>{0x2dU, 0x2dU, 0x2dU, 0x2dU, 0x2dU})));
    }

    void testFormatMismatchAndUnknownOutput() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto directory = el::path::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
        const auto certificate = X509Certificate::fromPemOrThrow(certificatePem());
        const auto badPemPath = directory->path() / "binary.pem"_el;
        badPemPath.content().writeDataOrThrow(certificate.toDer());
        REQUIRE_THROWS_AS(el::err::ParseError, X509Certificate::fromFileOrThrow(badPemPath));
        REQUIRE_THROWS_AS(
            el::err::ParameterFailure, certificate.writeToFile(directory->path() / "certificate.unknown"_el));
    }
};
