#pragma once

#include "MacosOwnedReference.hpp"
#include "TlsSigner.hpp"

#include "../keys/PublicKey.hpp"

#include "../../util/List.hpp"

#include <Security/Security.h>

namespace erbsland::cryptology::impl {

/// Native SecKey signer; only provider references and public capability data leave the user Keychain.
/// @tested{SystemIdentityStoreTest MacosSystemIdentityNativeTest}
class MacosTlsSigner final : public TlsSigner {
public:
    /// Take a native key reference and discover schemes compatible with the certificate.
    MacosTlsSigner(MacosOwnedReference<SecKeyRef> key, const PublicKey &publicKey, bool allowInteraction);

public: // implement TlsSigner
    [[nodiscard]] auto supports(TlsSignatureScheme scheme) const noexcept -> bool override;
    [[nodiscard]] auto sign(TlsSignatureScheme scheme, mem::ConstByteSpan message) const -> mem::ByteBlock override;
    [[nodiscard]] auto isNative() const noexcept -> bool override { return true; }

private:
    /// Map a supported TLS scheme to its exact Security.framework message-signature algorithm.
    [[nodiscard]] static auto algorithm(TlsSignatureScheme scheme) -> SecKeyAlgorithm;

private:
    MacosOwnedReference<SecKeyRef> _key;     ///< Native provider ownership; private bytes are never exported.
    util::List<TlsSignatureScheme> _schemes; ///< Discovered signing capabilities.
    bool _allowInteraction{};                ///< Explicit authorization-UI permission.
};

}
