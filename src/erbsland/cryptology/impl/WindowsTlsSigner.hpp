#pragma once

#include "TlsSigner.hpp"

#include "../keys/PublicKey.hpp"

#include "../../core/impl/WindowsApi.hpp"
#include "../../util/List.hpp"

#include <ncrypt.h>
#include <wincrypt.h>

namespace erbsland::cryptology::impl {

/// Current-user CNG signer with provider-owned private material and silent-by-default native calls.
/// @tested{SystemIdentityStoreTest WindowsSystemIdentityNativeTest}
class WindowsTlsSigner final : public TlsSigner {
public:
    /// Take ownership of an acquired CNG key and discover supported parameters by verified signing probes.
    WindowsTlsSigner(
        NCRYPT_KEY_HANDLE key,
        bool owned,
        bool allowInteraction,
        const PublicKey &publicKey,
        PCCERT_CONTEXT certificate);
    /// Release the acquired key and retained certificate in dependency order.
    ~WindowsTlsSigner() override;

    // defaults/deletions
    WindowsTlsSigner(const WindowsTlsSigner &) = delete;
    auto operator=(const WindowsTlsSigner &) -> WindowsTlsSigner & = delete;

public: // implement TlsSigner
    [[nodiscard]] auto supports(TlsSignatureScheme scheme) const noexcept -> bool override;
    [[nodiscard]] auto sign(TlsSignatureScheme scheme, mem::ConstByteSpan message) const -> mem::ByteBlock override;
    [[nodiscard]] auto isNative() const noexcept -> bool override { return true; }

private:
    /// Sign with exact native hash and padding parameters and normalize ECDSA output.
    [[nodiscard]] auto signNative(TlsSignatureScheme scheme, mem::ConstByteSpan message) const -> mem::ByteBlock;

private:
    NCRYPT_KEY_HANDLE _key{};      ///< Native CNG key; never exported.
    PCCERT_CONTEXT _certificate{}; ///< Retained certificate preserves borrowed cached-key lifetime.
    bool _owned{};                 ///< Whether CryptAcquireCertificatePrivateKey transferred handle ownership.
    bool _allowInteraction{};      ///< Explicit authorization-UI permission.
    util::List<TlsSignatureScheme> _schemes; ///< Successfully probed supported schemes.
};

}
