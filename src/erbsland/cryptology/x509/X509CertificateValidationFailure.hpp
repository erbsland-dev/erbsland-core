// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "X509Certificate.hpp"
#include "X509CertificateValidationFailureCategory.hpp"

#include "../impl/X509ServerCertificateValidator_fwd.hpp"

#include "../../text/String.hpp"
#include "../../util/List.hpp"

#include <optional>

namespace erbsland::cryptology {

/// Structured context for one rejected X.509 server-certificate validation.
/// @seedoc{/reference/cryptology/x509_certificates}
/// @tested{X509ServerCertificatePolicyTest}
class X509CertificateValidationFailure final {
    friend class impl::X509ServerCertificateValidator;

public: // accessors
    /// Get the stable failure category.
    [[nodiscard]] auto category() const noexcept -> X509CertificateValidationFailureCategory { return _category; }
    /// Get the certificate whose processing failed, or no value for an input-wide failure.
    [[nodiscard]] auto certificate() const noexcept -> const std::optional<X509Certificate> & { return _certificate; }
    /// Get the issuer candidate involved in an edge failure.
    [[nodiscard]] auto issuerCandidate() const noexcept -> const std::optional<X509Certificate> & {
        return _issuerCandidate;
    }
    /// Get the partial target-to-issuer path that led to this failure.
    [[nodiscard]] auto partialPath() const noexcept -> const util::List<X509Certificate> & { return _partialPath; }
    /// Get a human-readable diagnostic.
    [[nodiscard]] auto diagnostic() const noexcept -> const text::String & { return _diagnostic; }

private:
    /// Create structured validation-failure context.
    X509CertificateValidationFailure(
        X509CertificateValidationFailureCategory category,
        std::optional<X509Certificate> certificate,
        std::optional<X509Certificate> issuerCandidate,
        util::List<X509Certificate> partialPath,
        text::String diagnostic) noexcept :
        _category{category},
        _certificate{std::move(certificate)},
        _issuerCandidate{std::move(issuerCandidate)},
        _partialPath{std::move(partialPath)},
        _diagnostic{std::move(diagnostic)} {}

private:
    X509CertificateValidationFailureCategory _category{
        X509CertificateValidationFailureCategory::EmptyPeerCertificates}; ///< Stable failure category.
    std::optional<X509Certificate> _certificate;                          ///< Optional affected certificate.
    std::optional<X509Certificate> _issuerCandidate;                      ///< Optional issuer candidate.
    util::List<X509Certificate> _partialPath;                             ///< Partial target-to-issuer path.
    text::String _diagnostic;                                             ///< Human-readable diagnostic.
};

}
