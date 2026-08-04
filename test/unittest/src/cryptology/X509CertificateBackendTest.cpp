// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "X509TestData.hpp"

#include <erbsland/cryptology/impl/PortableX509CertificateData.hpp>
#include <erbsland/cryptology/impl/X509CertificateBackendAccess.hpp>
#include <erbsland/cryptology/impl/X509CertificateData.hpp>
#include <erbsland/cryptology/x509/X509Certificate.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstddef>

using namespace el::cryptology;
using erbsland::test::x509::certificatePem;

class MockX509CertificateData final : public el::cryptology::impl::X509CertificateData {
public:
    explicit MockX509CertificateData(const el::cryptology::impl::PortableX509CertificateData &portableData) :
        _portableData{portableData} {}

    [[nodiscard]] auto portableData() const -> const el::cryptology::impl::PortableX509CertificateData & override {
        ++_accessCount;
        return _portableData;
    }

    [[nodiscard]] auto clone() const -> X509CertificateData * override {
        return new MockX509CertificateData{_portableData};
    }

    [[nodiscard]] auto accessCount() const noexcept -> std::size_t { return _accessCount; }

private:
    el::cryptology::impl::PortableX509CertificateData _portableData;
    mutable std::size_t _accessCount{};
};

TESTED_TARGETS(X509Certificate)
class X509CertificateBackendTest final : public el::UnitTest {
public:
    void testPlatformBackendFacadeContract() {
        const auto portableCertificate = X509Certificate::fromPemOrThrow(certificatePem());
        const auto *portableData =
            el::cryptology::impl::X509CertificateBackendAccess{portableCertificate}.portableData();
        REQUIRE(portableData != nullptr);
        auto *mockData = new MockX509CertificateData{*portableData};
        auto dataPointer = el::cryptology::impl::X509CertificateDataPtr{mockData};
        const auto certificate = el::cryptology::impl::X509CertificateBackendAccess{std::move(dataPointer)}.create();
        REQUIRE_FALSE(certificate.isEmpty());
        REQUIRE_EQUAL(certificate.toDer(), portableCertificate.toDer());
        REQUIRE_EQUAL(certificate.subject().toString(), portableCertificate.subject().toString());
        REQUIRE(mockData->accessCount() >= 2U);
    }
};
