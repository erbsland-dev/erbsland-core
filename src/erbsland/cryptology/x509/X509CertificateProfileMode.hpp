// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::cryptology {

/// The handling of independently detectable RFC 5280 profile violations.
enum class X509CertificateProfileMode : uint8_t {
    Strict,     ///< Reject certificates with detected profile violations.
    Compatible, ///< Retain reviewed profile anomalies and report typed issues.
};

}
