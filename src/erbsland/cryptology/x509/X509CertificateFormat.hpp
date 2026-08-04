// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::cryptology {

/// The external encoding of an X.509 certificate or certificate bundle.
enum class X509CertificateFormat : uint8_t {
    Automatic, ///< Detect the input or select output from a recognized suffix.
    Pem,       ///< RFC 7468 textual encoding.
    Der,       ///< Canonical ASN.1 distinguished encoding rules.
};

}
