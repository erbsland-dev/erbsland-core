// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "X509Certificate.hpp"
#include "X509CertificateValidationFailure.hpp"

#include "../impl/X509ServerCertificateValidator_fwd.hpp"

#include "../../util/List.hpp"

#include <optional>

namespace erbsland::cryptology {

/// The explicit result of X.509 server-certificate validation.
/// This type deliberately has no boolean conversion. An accepted result carries the selected path in target-to-anchor
/// order; a rejected result carries structured failure context and never uses an empty certificate as a signal.
/// @seedoc{/reference/cryptology/x509_certificates}
/// @tested{X509ServerCertificatePolicyTest}
class X509CertificateValidation final {
    friend class impl::X509ServerCertificateValidator;

public: // tests
    /// Test whether the certificate was accepted for the requested server identity.
    [[nodiscard]] auto isAccepted() const noexcept -> bool { return _accepted; }
    /// Test whether the certificate was rejected.
    [[nodiscard]] auto isRejected() const noexcept -> bool { return !_accepted; }

public: // accessors
    /// Get the selected target-to-anchor path, or an empty list after rejection.
    [[nodiscard]] auto validatedPath() const noexcept -> const util::List<X509Certificate> & { return _validatedPath; }
    /// Get structured failure context, or no value after acceptance.
    [[nodiscard]] auto failure() const noexcept -> const std::optional<X509CertificateValidationFailure> & {
        return _failure;
    }

private:
    /// Create an accepted validation result.
    explicit X509CertificateValidation(util::List<X509Certificate> validatedPath) noexcept :
        _accepted{true}, _validatedPath{std::move(validatedPath)} {}
    /// Create a rejected validation result.
    explicit X509CertificateValidation(X509CertificateValidationFailure failure) noexcept :
        _failure{std::move(failure)} {}

private:
    bool _accepted{};                                         ///< Whether validation was accepted.
    util::List<X509Certificate> _validatedPath;               ///< Selected target-to-anchor path.
    std::optional<X509CertificateValidationFailure> _failure; ///< Structured rejection context.
};

}
