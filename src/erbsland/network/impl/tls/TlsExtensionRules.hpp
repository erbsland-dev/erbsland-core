#pragma once

#include <cstdint>

/// Context checks for ignored extensions in RFC 8446 sections 4.2, 4.3.2 and 4.6.1.
namespace erbsland::network::impl::tls_extension_rules {

/// Reject known extensions forbidden in CertificateRequest; ignore unrecognized extensions.
/// @param type Extension wire type.
/// @throws TlsProtocolError If a known extension occurs in a prohibited context.
/// @tested{TlsClientProtocolTest}
void requireIgnorableCertificateRequest(uint16_t type);
/// Reject known extensions forbidden in NewSessionTicket; ignore unrecognized extensions.
/// @param type Extension wire type.
/// @throws TlsProtocolError If a known extension occurs in a prohibited context.
/// @tested{TlsClientProtocolTest}
void requireIgnorableSessionTicket(uint16_t type);

/// Test whether RFC 8446 assigns this extension a message context.
/// @tested{TlsClientProtocolTest}
[[nodiscard]] auto isKnown(uint16_t type) noexcept -> bool;

}
