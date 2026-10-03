// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TlsConfigurationParser_fwd.hpp"
#include "TlsConfigurationSet.hpp"

#include "../keys/SigningPrivateKey_fwd.hpp"
#include "../x509/X509CertificateBundle_fwd.hpp"

#include "../../conf/Integer.hpp"
#include "../../conf/Value_fwd.hpp"
#include "../../conf/vr/Rules_fwd.hpp"
#include "../../path/Path_fwd.hpp"
#include "../../text/String_fwd.hpp"
#include "../../text/StringLiteral.hpp"

namespace erbsland::cryptology {

/// Parse shared TLS settings and role-specific profile lists.
/// @seedoc{/topics/cryptology/configuring_tls}
/// @tested{TlsConfigurationParserTest}
class TlsConfigurationParser final {
    constexpr static auto cSystemTrustValue = text::StringLiteral{"system_trust"};

public:
    /// Access the compiled validation rules shared by all parser instances.
    /// @return The immutable compiled rules for the complete TLS section.
    [[nodiscard]] static auto validationRules() -> const conf::vr::RulesPtr &;
    /// Get the current version of the TLS configuration format.
    [[nodiscard]] static auto version() -> conf::Integer;
    /// Validate and load a complete TLS section without modifying the application registry.
    /// @param sectionValue The optional TLS section; null produces the default empty configuration set.
    /// @return Owned shared settings and grouped role profiles.
    /// @throws conf::ConfError If validation or referenced-material loading fails.
    [[nodiscard]] auto parse(const conf::ValuePtr &sectionValue) const -> TlsConfigurationSet;

private:
    /// Load an explicitly configured role-neutral file or native identity.
    [[nodiscard]] static auto loadIdentity(const conf::ValuePtr &section, bool allowInteraction)
        -> std::optional<TlsIdentity>;
    /// Load a strict PEM or DER certificate bundle from a file field.
    /// @param value The validated file field.
    /// @param errorMessage The diagnostic description used if loading fails.
    /// @return The loaded certificate bundle.
    /// @throws conf::ConfError If resolving, reading, or parsing the file fails.
    [[nodiscard]] static auto loadCertificateBundle(const conf::ValuePtr &value, const text::String &errorMessage)
        -> X509CertificateBundle;
    /// Load a PKCS#8 private key; password presence selects encrypted input.
    /// @param value The validated file field.
    /// @param password The optional exact password, including a valid empty value.
    /// @return The loaded private key.
    /// @throws conf::ConfError If resolving, reading, or parsing the file fails.
    [[nodiscard]] static auto loadSigningPrivateKey(const conf::ValuePtr &value, const conf::ValuePtr &password)
        -> SigningPrivateKey;
    /// Resolve one file field relative to the ELCL source that defines it.
    /// @param value The validated text value containing a file reference.
    /// @return The absolute or already-absolute material path.
    /// @throws conf::ConfError If a relative reference has no file source or the path is invalid.
    [[nodiscard]] static auto resolveFile(const conf::ValuePtr &value) -> path::Path;
};

}
