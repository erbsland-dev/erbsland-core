// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::cryptology {

/// A safe web-certificate extension and validation profile.
enum class X509CertificateProfile : uint8_t {
    CertificateAuthority, ///< Certificate authority that may issue certificates and CRLs.
    TlsServer,            ///< TLS server identity requiring a DNS or IP subject alternative name.
    TlsClient,            ///< TLS client identity with an optional subject alternative name.
    TlsServerAndClient,   ///< Dual-use TLS identity requiring a DNS or IP subject alternative name.
};

}
