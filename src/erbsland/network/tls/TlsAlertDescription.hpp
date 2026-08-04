// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::network {

/// A TLS alert description preserved as its public RFC wire value.
/// Unknown peer values can be represented by casting their received byte to this type.
enum class TlsAlertDescription : uint8_t {
    CloseNotify = 0U,                    ///< Orderly TLS closure.
    UnexpectedMessage = 10U,             ///< Message was inappropriate for the current state.
    BadRecordMac = 20U,                  ///< Record authentication failed.
    RecordOverflow = 22U,                ///< Record exceeded the protocol limit.
    HandshakeFailure = 40U,              ///< No acceptable security parameters were negotiated.
    BadCertificate = 42U,                ///< Certificate processing failed generically.
    UnsupportedCertificate = 43U,        ///< Certificate type or algorithm is unsupported.
    CertificateRevoked = 44U,            ///< Certificate was revoked.
    CertificateExpired = 45U,            ///< Certificate is outside its validity period.
    CertificateUnknown = 46U,            ///< Another certificate failure occurred.
    IllegalParameter = 47U,              ///< A field was inconsistent with negotiated parameters.
    UnknownCa = 48U,                     ///< No trusted certification path was found.
    AccessDenied = 49U,                  ///< Access was denied after authentication.
    DecodeError = 50U,                   ///< A field could not be decoded completely.
    DecryptError = 51U,                  ///< A handshake signature or Finished value failed.
    ProtocolVersion = 70U,               ///< The peer did not negotiate a supported protocol version.
    InsufficientSecurity = 71U,          ///< Negotiated parameters did not meet security policy.
    InternalError = 80U,                 ///< A local internal operation failed.
    InappropriateFallback = 86U,         ///< A protocol fallback was inappropriate.
    UserCanceled = 90U,                  ///< An operation was cancelled by its initiator.
    MissingExtension = 109U,             ///< A mandatory extension was absent.
    UnsupportedExtension = 110U,         ///< A forbidden or unsolicited extension was received.
    UnrecognizedName = 112U,             ///< The requested server name was not recognized.
    BadCertificateStatusResponse = 113U, ///< Certificate status response validation failed.
    UnknownPskIdentity = 115U,           ///< A pre-shared key identity was unknown.
    CertificateRequired = 116U,          ///< A required certificate was not supplied.
    NoApplicationProtocol = 120U,        ///< No acceptable ALPN protocol was negotiated.
};

}
