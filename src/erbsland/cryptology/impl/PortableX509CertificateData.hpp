// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PortableX509CertificateValues.hpp"
#include "X509CertificateData.hpp"

namespace erbsland::cryptology::impl {

/// Immutable portable representation of a parsed X.509 certificate.
/// @tested{X509CertificateTest X509CertificateFileTest}
class PortableX509CertificateData final : public X509CertificateData {
public:
    /// Create portable data from parsed values.
    explicit PortableX509CertificateData(PortableX509CertificateValues values) noexcept : _values{std::move(values)} {}

    // defaults
    PortableX509CertificateData(const PortableX509CertificateData &) = default;

public: // accessors
    /// Access all parsed values.
    [[nodiscard]] auto values() const noexcept -> const PortableX509CertificateValues & { return _values; }

public: // implement X509CertificateData
    [[nodiscard]] auto portableData() const -> const PortableX509CertificateData & override { return *this; }
    [[nodiscard]] auto clone() const -> X509CertificateData * override {
        return new PortableX509CertificateData{*this};
    }

private:
    PortableX509CertificateValues _values; ///< Complete parsed values.
};

}
