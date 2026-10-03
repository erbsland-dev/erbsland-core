#include "MacosTlsSigner.hpp"

#include "MacosKeychainInteraction.hpp"
#include "NativeTlsAlgorithms.hpp"

#include "../CryptologyError.hpp"

#include "../../text/Literals.hpp"

namespace erbsland::cryptology::impl {

using namespace text::literals;

MacosTlsSigner::MacosTlsSigner(
    MacosOwnedReference<SecKeyRef> key, const PublicKey &publicKey, const bool allowInteraction) :
    _key{std::move(key)}, _allowInteraction{allowInteraction} {
    auto schemes = std::vector<TlsSignatureScheme>{};
    for (const auto scheme : native_tls_algorithms::schemes()) {
        if (native_tls_algorithms::matches(publicKey, scheme) &&
            SecKeyIsAlgorithmSupported(_key.get(), kSecKeyOperationTypeSign, algorithm(scheme))) {
            schemes.push_back(scheme);
        }
    }
    _schemes = util::List<TlsSignatureScheme>{std::move(schemes)};
}

auto MacosTlsSigner::supports(const TlsSignatureScheme scheme) const noexcept -> bool {
    for (const auto candidate : _schemes) {
        if (candidate == scheme) {
            return true;
        }
    }
    return false;
}

auto MacosTlsSigner::sign(const TlsSignatureScheme scheme, const mem::ConstByteSpan message) const -> mem::ByteBlock {
    if (!supports(scheme)) {
        throw CryptologyError{"The native key does not support this TLS signature scheme."_el};
    }
    const auto data = MacosOwnedReference<CFDataRef>{CFDataCreate(
        kCFAllocatorDefault, reinterpret_cast<const UInt8 *>(message.data()), static_cast<CFIndex>(message.size()))};
    auto error = MacosOwnedReference<CFErrorRef>{};
    // RFC 8446 section 4.2.3: Security.framework message algorithms hash once, use digest-sized PSS salt,
    // and return X9.62 ECDSA DER. TlsIdentity subsequently verifies output against the certificate and exact scheme.
    auto signature = MacosOwnedReference<CFDataRef>{};
    {
        const auto interaction = MacosKeychainInteraction{_allowInteraction};
        if (interaction.status() != errSecSuccess) {
            throw CryptologyError{"The Keychain interaction policy could not be applied."_el};
        }
        signature.reset(SecKeyCreateSignature(_key.get(), algorithm(scheme), data.get(), error.put()));
    }
    if (signature.get() == nullptr) {
        throw CryptologyError{"The Keychain could not authorize or complete TLS signing."_el};
    }
    return mem::ByteBlock::fromSpan(
        std::span<const uint8_t>{
            CFDataGetBytePtr(signature.get()), static_cast<std::size_t>(CFDataGetLength(signature.get()))});
}

auto MacosTlsSigner::algorithm(const TlsSignatureScheme scheme) -> SecKeyAlgorithm {
    switch (scheme.toRawValue()) {
    case 0x0403U:
        return kSecKeyAlgorithmECDSASignatureMessageX962SHA256;
    case 0x0503U:
        return kSecKeyAlgorithmECDSASignatureMessageX962SHA384;
    case 0x0804U:
    case 0x0809U:
        return kSecKeyAlgorithmRSASignatureMessagePSSSHA256;
    case 0x0805U:
    case 0x080aU:
        return kSecKeyAlgorithmRSASignatureMessagePSSSHA384;
    case 0x0806U:
    case 0x080bU:
        return kSecKeyAlgorithmRSASignatureMessagePSSSHA512;
    default:
        throw CryptologyError{"Unsupported Keychain TLS signature algorithm."_el};
    }
}

}
