#include "WindowsTlsSigner.hpp"

#include "NativeTlsAlgorithms.hpp"

#include "../CryptologyError.hpp"
#include "../Hasher.hpp"

#include "../../err/RuntimeError.hpp"
#include "../../text/Literals.hpp"

#include <bcrypt.h>

#include <vector>

namespace erbsland::cryptology::impl {

using namespace mem;
using namespace text::literals;

WindowsTlsSigner::WindowsTlsSigner(
    const NCRYPT_KEY_HANDLE key,
    const bool owned,
    const bool allowInteraction,
    const PublicKey &publicKey,
    const PCCERT_CONTEXT certificate) :
    _key{key},
    _certificate{CertDuplicateCertificateContext(certificate)},
    _owned{owned},
    _allowInteraction{allowInteraction} {
    try {
        auto schemes = std::vector<TlsSignatureScheme>{};
        for (const auto scheme : native_tls_algorithms::schemes()) {
            if (!native_tls_algorithms::matches(publicKey, scheme)) {
                continue;
            }
            try {
                // Provider capability is not inferred from the algorithm name: validate a harmless empty-message
                // probe with the exact salt/hash combination before this scheme can be advertised or selected.
                const auto signature = signNative(scheme, {});
                if (publicKey.verifyTlsCertificateVerifySignature(scheme, {}, signature.span())) {
                    schemes.push_back(scheme);
                }
            } catch (const err::RuntimeError &) {}
        }
        _schemes = util::List<TlsSignatureScheme>{std::move(schemes)};
    } catch (...) {
        if (_owned) {
            NCryptFreeObject(_key);
        }
        if (_certificate != nullptr) {
            CertFreeCertificateContext(_certificate);
        }
        throw;
    }
}

WindowsTlsSigner::~WindowsTlsSigner() {
    if (_owned) {
        NCryptFreeObject(_key);
    }
    if (_certificate != nullptr) {
        CertFreeCertificateContext(_certificate);
    }
}

auto WindowsTlsSigner::supports(const TlsSignatureScheme scheme) const noexcept -> bool {
    for (const auto candidate : _schemes) {
        if (candidate == scheme) {
            return true;
        }
    }
    return false;
}

auto WindowsTlsSigner::sign(const TlsSignatureScheme scheme, const ConstByteSpan message) const -> ByteBlock {
    if (!supports(scheme)) {
        throw CryptologyError{"The CNG key does not support the selected TLS scheme."_el};
    }
    return signNative(scheme, message);
}

auto WindowsTlsSigner::signNative(const TlsSignatureScheme scheme, const ConstByteSpan message) const -> ByteBlock {
    const auto algorithm = native_tls_algorithms::hash(scheme);
    auto hasher = Hasher{algorithm};
    hasher.update(message);
    const auto digest = hasher.finalize();
    auto padding = BCRYPT_PSS_PADDING_INFO{};
    padding.pszAlgId = BCRYPT_SHA256_ALGORITHM;
    if (algorithm == HashAlgorithm::Sha2_384) {
        padding.pszAlgId = BCRYPT_SHA384_ALGORITHM;
    }
    if (algorithm == HashAlgorithm::Sha2_512) {
        padding.pszAlgId = BCRYPT_SHA512_ALGORITHM;
    }
    padding.cbSalt = static_cast<ULONG>(digest.length().toSizeT());
    const auto isRsa = scheme.toRawValue() >= 0x0804U;
    auto flags = DWORD{};
    if (!_allowInteraction) {
        flags |= NCRYPT_SILENT_FLAG;
    }
    if (isRsa) {
        flags |= NCRYPT_PAD_PSS_FLAG;
    }
    auto paddingInfo = static_cast<void *>(nullptr);
    if (isRsa) {
        paddingInfo = &padding;
    }
    auto size = DWORD{};
    auto digestBytes = const_cast<PBYTE>(reinterpret_cast<const BYTE *>(digest.span().data()));
    // RFC 8446 section 4.2.3 and RFC 8017 section 9.1: SHA-2 prehash once, MGF1 uses the same hash and saltLen=hLen.
    if (NCryptSignHash(
            _key, paddingInfo, digestBytes, static_cast<DWORD>(digest.length().toSizeT()), nullptr, 0, &size, flags) !=
            ERROR_SUCCESS ||
        size == 0 || size > 16384) {
        throw CryptologyError{"CNG signing is unavailable, unsupported, or requires authorization."_el};
    }
    auto result = std::vector<uint8_t>(size);
    if (NCryptSignHash(
            _key,
            paddingInfo,
            digestBytes,
            static_cast<DWORD>(digest.length().toSizeT()),
            result.data(),
            size,
            &size,
            flags) != ERROR_SUCCESS ||
        size > result.size()) {
        throw CryptologyError{"CNG could not authorize or complete TLS signing."_el};
    }
    result.resize(size);
    const auto signature = ByteBlock::fromVector(result);
    if (!isRsa) {
        return native_tls_algorithms::ecdsaDer(signature.span());
    }
    return signature;
}

}
