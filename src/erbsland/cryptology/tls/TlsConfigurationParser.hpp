// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TlsConfigurationEntry.hpp"
#include "TlsConfigurationParser_fwd.hpp"

#include "../keys/SigningPrivateKey_fwd.hpp"
#include "../x509/X509CertificateBundle_fwd.hpp"

#include "../../conf/Integer.hpp"
#include "../../conf/Value_fwd.hpp"
#include "../../conf/vr/Rules_fwd.hpp"
#include "../../path/Path_fwd.hpp"
#include "../../text/String_fwd.hpp"

namespace erbsland::cryptology {

/// Parse and validate one labeled TLS configuration from an ELCL section.
/// @seedoc{/topics/cryptology/configuring_tls}
/// @tested{TlsConfigurationParserTest}
class TlsConfigurationParser final {
public:
    /// Access the compiled validation rules shared by all parser instances.
    /// @return The immutable compiled rules for one TLS configuration entry.
    [[nodiscard]] static auto validationRules() -> const conf::vr::RulesPtr &;
    /// Get the current version of the TLS configuration format.
    [[nodiscard]] static auto version() -> conf::Integer;
    /// Validate and parse one TLS configuration section-list entry.
    /// @param sectionValue The selected ELCL section containing one labeled TLS configuration.
    /// @return The validated label and complete TLS configuration.
    /// @throws conf::ConfError If validation or referenced-material loading fails.
    /// @throws err::ParameterError If `sectionValue` is null.
    [[nodiscard]] auto parse(const conf::ValuePtr &sectionValue) const -> TlsConfigurationEntry;

private:
    /// Load a strict PEM or DER certificate bundle from a file field.
    /// @param value The validated file field.
    /// @param errorMessage The diagnostic description used if loading fails.
    /// @return The loaded certificate bundle.
    /// @throws conf::ConfError If resolving, reading, or parsing the file fails.
    [[nodiscard]] static auto loadCertificateBundle(const conf::ValuePtr &value, const text::String &errorMessage)
        -> X509CertificateBundle;
    /// Load an unencrypted PKCS#8 private key from a file field.
    /// @param value The validated file field.
    /// @return The loaded private key.
    /// @throws conf::ConfError If resolving, reading, or parsing the file fails.
    [[nodiscard]] static auto loadSigningPrivateKey(const conf::ValuePtr &value) -> SigningPrivateKey;
    /// Resolve one file field relative to the ELCL source that defines it.
    /// @param value The validated text value containing a file reference.
    /// @return The absolute or already-absolute material path.
    /// @throws conf::ConfError If a relative reference has no file source or the path is invalid.
    [[nodiscard]] static auto resolveFile(const conf::ValuePtr &value) -> path::Path;
};

}
