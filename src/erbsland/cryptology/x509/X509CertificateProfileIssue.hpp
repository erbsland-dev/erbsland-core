// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "X509CertificateProfileIssueCategory.hpp"

#include "../asn1/Asn1ObjectIdentifier.hpp"

#include "../../text/String.hpp"

namespace erbsland::cryptology {

/// A machine-readable X.509 profile issue retained in compatible mode.
/// @tested{X509CertificateTest}
class X509CertificateProfileIssue final {
public:
    /// Create an empty issue.
    X509CertificateProfileIssue() = default;
    /// Create a profile issue.
    /// @param category The stable issue category.
    /// @param field The affected field name.
    /// @param oid The affected object identifier, if any.
    /// @param diagnostic A human-readable diagnostic.
    X509CertificateProfileIssue(
        X509CertificateProfileIssueCategory category,
        text::String field,
        Asn1ObjectIdentifier oid,
        text::String diagnostic) noexcept :
        _category{category}, _field{std::move(field)}, _oid{std::move(oid)}, _diagnostic{std::move(diagnostic)} {}

public: // accessors
    /// Get the stable issue category.
    [[nodiscard]] auto category() const noexcept -> X509CertificateProfileIssueCategory { return _category; }
    /// Get the affected field name.
    [[nodiscard]] auto field() const noexcept -> const text::String & { return _field; }
    /// Get the affected object identifier, if any.
    [[nodiscard]] auto oid() const noexcept -> const Asn1ObjectIdentifier & { return _oid; }
    /// Get the diagnostic text.
    [[nodiscard]] auto diagnostic() const noexcept -> const text::String & { return _diagnostic; }

private:
    X509CertificateProfileIssueCategory _category{X509CertificateProfileIssueCategory::SerialNumber}; ///< Category.
    text::String _field;                                                                              ///< Field name.
    Asn1ObjectIdentifier _oid;                                                                        ///< Optional OID.
    text::String _diagnostic;                                                                         ///< Diagnostic.
};

}
