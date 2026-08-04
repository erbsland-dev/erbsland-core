// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::cryptology {

/// A stable category for a retained X.509 profile issue.
enum class X509CertificateProfileIssueCategory : uint8_t {
    SerialNumber,               ///< The serial number violates the RFC 5280 profile.
    SignatureAlgorithmMismatch, ///< Inner and outer signature algorithms differ.
    VersionField,               ///< A field is inconsistent with the certificate version.
    ValidityRange,              ///< The not-after time precedes the not-before time.
    ExtensionValue,             ///< A recognized extension has a malformed inner value.
    NameValue,                  ///< A name contains an unsupported or malformed value.
};

}
