// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../impl/X509Parser_fwd.hpp"

#include <cstdint>
#include <optional>

namespace erbsland::cryptology {

/// Decoded X.509 Basic Constraints.
/// @tested{X509CertificateTest}
class X509BasicConstraints final {
    friend class impl::X509Parser;

public:
    /// Create constraints for an end-entity certificate.
    X509BasicConstraints() = default;

public: // accessors
    /// Test if the subject may act as a certificate authority.
    [[nodiscard]] auto isCertificateAuthority() const noexcept -> bool { return _certificateAuthority; }
    /// Get the optional maximum subordinate CA depth.
    [[nodiscard]] auto pathLength() const noexcept -> std::optional<uint32_t> { return _pathLength; }

private:
    /// Create parsed basic-constraints values.
    X509BasicConstraints(bool certificateAuthority, std::optional<uint32_t> pathLength) noexcept :
        _certificateAuthority{certificateAuthority}, _pathLength{pathLength} {}

private:
    bool _certificateAuthority{};        ///< Whether the subject is a CA.
    std::optional<uint32_t> _pathLength; ///< Optional subordinate CA depth.
};

}
