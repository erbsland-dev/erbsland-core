#include "TlsServerProtocol.hpp"

#include "../TlsProtocolError.hpp"
#include "../TlsWireWriter.hpp"

#include "../../../../err/RuntimeError.hpp"
#include "../../../../text/Literals.hpp"
#include "../../../../unit/ByteCount.hpp"

namespace erbsland::network::impl {

using namespace text::literals;

void TlsServerProtocol::completeSignature(mem::ByteBlock signature) noexcept {
    if (_checkpoint != TlsServerProtocolCheckpoint::Signature || !_pendingSignature.has_value()) {
        return;
    }
    try {
        if (signature.isEmpty() || signature.length() > unit::ByteCount{65535U}) {
            throw TlsProtocolError{
                TlsAlertDescription::InternalError, "A TLS signer returned an invalid signature size."_el};
        }
        _checkpoint = TlsServerProtocolCheckpoint::None;
        finishSignedFlight(signature);
        _pendingSignature.reset();
        processRetainedInputNoThrow();
    } catch (const TlsProtocolError &error) {
        fail(error.alert(), error.reason());
    } catch (const err::RuntimeError &error) {
        fail(TlsAlertDescription::InternalError, error.reason());
    }
}

void TlsServerProtocol::failPendingOperation(text::String diagnostic) noexcept {
    if (_state == TlsServerProtocolState::Handshaking) {
        fail(TlsAlertDescription::InternalError, std::move(diagnostic));
    }
}

}
