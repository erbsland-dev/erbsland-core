#include "SystemIdentityStore.hpp"

#include "../impl/SystemIdentityBackend.hpp"

#include "../../err/ParameterFailure.hpp"
#include "../../text/Literals.hpp"
#include "../../unit/ByteCount.hpp"

namespace erbsland::cryptology {

SystemIdentityStore::SystemIdentityStore(const bool allowInteraction) :
    _backend{impl::SystemIdentityBackend::create(allowInteraction)} {
}

auto SystemIdentityStore::isSupported() const noexcept -> bool {
    return _backend->isSupported();
}

auto SystemIdentityStore::list() const -> util::List<SystemIdentityInfo> {
    return _backend->list();
}

auto SystemIdentityStore::load(const mem::ByteBlock &fingerprint) const -> TlsIdentity {
    using namespace text::literals;
    if (fingerprint.length() != unit::ByteCount{32U}) {
        throw err::ParameterFailure{
            "An identity fingerprint must contain exactly 32 SHA-256 bytes."_el, "fingerprint"_el};
    }
    auto material = _backend->load(fingerprint);
    return TlsIdentity{std::move(material.chain), std::move(material.signer)};
}

}
