// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "X509CertificateData.hpp"

#include "../x509/X509Certificate.hpp"

namespace erbsland::cryptology::impl {

/// Internal bridge used by platform certificate-store implementations.
/// @tested{X509CertificateBackendTest}
class X509CertificateBackendAccess final {
public:
    /// Create access for certificate data that shall be wrapped in a facade.
    explicit X509CertificateBackendAccess(X509CertificateDataPtr data) noexcept : _data{std::move(data)} {}
    /// Create access for an existing certificate facade.
    explicit X509CertificateBackendAccess(X509Certificate certificate) noexcept :
        _certificate{std::move(certificate)} {}

public:
    /// Create a certificate facade around portable or platform-specific shared data.
    [[nodiscard]] auto create() noexcept -> X509Certificate { return X509Certificate{std::move(_data)}; }
    /// Access the materialized portable representation for backend integration tests and adapters.
    [[nodiscard]] auto portableData() const -> const PortableX509CertificateData * {
        return _certificate.portableData();
    }

private:
    X509CertificateDataPtr _data; ///< Data to wrap in a certificate facade.
    X509Certificate _certificate; ///< Certificate facade to inspect.
};

}
