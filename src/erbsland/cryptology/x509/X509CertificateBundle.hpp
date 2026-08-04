// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "X509Certificate.hpp"

#include "../../path/Path_fwd.hpp"
#include "../../util/List.hpp"

namespace erbsland::cryptology {

/// An ordered bundle of X.509 certificates.
/// PEM supports multiple certificates. DER conversion and output require exactly one certificate.
/// @seedoc{/reference/cryptology/x509_certificates}
/// @tested{X509CertificateTest X509CertificateFileTest}
class X509CertificateBundle final {
public:
    /// Create an empty certificate bundle.
    X509CertificateBundle() = default;
    /// Create a bundle from ordered certificates.
    /// @param certificates The certificates to store; empty certificate elements are rejected.
    /// @throws err::ParameterError If an element is empty or the fixed count limit is exceeded.
    explicit X509CertificateBundle(util::List<X509Certificate> certificates);

public: // tests and accessors
    /// Test if the bundle contains no certificates.
    [[nodiscard]] auto isEmpty() const noexcept -> bool { return _certificates.isEmpty(); }
    /// Get certificates in source order.
    [[nodiscard]] auto certificates() const noexcept -> const util::List<X509Certificate> & { return _certificates; }

public: // conversion
    /// Encode every certificate as adjacent canonical PEM blocks.
    [[nodiscard]] auto toPem() const -> text::String;
    /// Return DER when the bundle contains exactly one certificate.
    /// @throws err::LogicError If the bundle does not contain exactly one certificate.
    [[nodiscard]] auto toDer() const -> mem::ByteBlock;
    /// Write this bundle to a file.
    /// @throws err::LogicError If DER is selected for a bundle not containing exactly one certificate.
    /// @throws err::ParameterError If automatic output cannot select a format.
    /// @throws path::PathError If writing fails.
    void writeToFile(const path::Path &path, X509CertificateFormat format = X509CertificateFormat::Automatic) const;

public: // factories
    /// Parse one or more strict CERTIFICATE PEM blocks, returning an empty bundle on any error.
    [[nodiscard]] static auto fromPem(
        const text::String &text, X509CertificateProfileMode mode = X509CertificateProfileMode::Strict) noexcept
        -> X509CertificateBundle;
    /// Parse one or more strict CERTIFICATE PEM blocks.
    [[nodiscard]] static auto fromPemOrThrow(
        const text::String &text, X509CertificateProfileMode mode = X509CertificateProfileMode::Strict)
        -> X509CertificateBundle;
    /// Parse one DER certificate as a one-element bundle, returning an empty bundle on any error.
    [[nodiscard]] static auto fromDer(
        const mem::ByteBlock &data, X509CertificateProfileMode mode = X509CertificateProfileMode::Strict) noexcept
        -> X509CertificateBundle;
    /// Parse one DER certificate as a one-element bundle.
    [[nodiscard]] static auto fromDerOrThrow(
        const mem::ByteBlock &data, X509CertificateProfileMode mode = X509CertificateProfileMode::Strict)
        -> X509CertificateBundle;
    /// Read a certificate bundle from a file, returning an empty bundle on any error.
    [[nodiscard]] static auto fromFile(
        const path::Path &path,
        X509CertificateFormat format = X509CertificateFormat::Automatic,
        X509CertificateProfileMode mode = X509CertificateProfileMode::Strict) noexcept -> X509CertificateBundle;
    /// Read a certificate bundle from a file.
    [[nodiscard]] static auto fromFileOrThrow(
        const path::Path &path,
        X509CertificateFormat format = X509CertificateFormat::Automatic,
        X509CertificateProfileMode mode = X509CertificateProfileMode::Strict) -> X509CertificateBundle;

private:
    util::List<X509Certificate> _certificates; ///< Certificates in source order.
};

}
