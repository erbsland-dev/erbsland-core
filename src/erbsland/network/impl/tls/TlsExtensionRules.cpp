#include "TlsExtensionRules.hpp"

#include "TlsProtocolError.hpp"

#include "../../../text/Literals.hpp"

namespace erbsland::network::impl::tls_extension_rules {

using namespace text::literals;

auto isKnown(const uint16_t type) noexcept -> bool {
    // RFC 8446 section 4.2 table: these extensions have defined TLS 1.3 message contexts.
    switch (type) {
    case 0U:
    case 1U:
    case 5U:
    case 10U:
    case 13U:
    case 14U:
    case 15U:
    case 16U:
    case 18U:
    case 19U:
    case 20U:
    case 21U:
    case 41U:
    case 42U:
    case 43U:
    case 44U:
    case 45U:
    case 47U:
    case 48U:
    case 49U:
    case 50U:
    case 51U:
        return true;
    default:
        return false;
    }
}

void requireIgnorableCertificateRequest(const uint16_t type) {
    // RFC 8446 section 4.3.2: unknown capabilities are ignored, not promoted into required client behavior.
    if (isKnown(type) && type != 5U && type != 13U && type != 18U && type != 47U && type != 48U && type != 50U) {
        throw TlsProtocolError{
            TlsAlertDescription::IllegalParameter, "An extension is forbidden in CertificateRequest."_el};
    }
}

void requireIgnorableSessionTicket(const uint16_t type) {
    // RFC 8446 section 4.6.1: only early_data has a defined ticket context; unknown extensions are ignored.
    if (isKnown(type) && type != 42U) {
        throw TlsProtocolError{
            TlsAlertDescription::IllegalParameter, "An extension is forbidden in NewSessionTicket."_el};
    }
}

}
