// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TlsCipherSuite.hpp"

#include "../../err/ParseError.hpp"
#include "../../text/Literals.hpp"

#include <array>

namespace erbsland::cryptology {

using namespace text::literals;

auto TlsCipherSuite::hashAlgorithm() const noexcept -> HashAlgorithm {
    return _value == Aes256GcmSha384 ? HashAlgorithm::Sha2_384 : HashAlgorithm::Sha2_256;
}

auto TlsCipherSuite::encryptionType() const noexcept -> SymmetricEncryptionType {
    switch (_value) {
    case Aes128GcmSha256:
        return SymmetricEncryptionType::Aes128Gcm;
    case Aes256GcmSha384:
        return SymmetricEncryptionType::Aes256Gcm;
    case ChaCha20Poly1305Sha256:
        return SymmetricEncryptionType::ChaCha20Poly1305;
    }
    return {};
}

auto TlsCipherSuite::toString() const -> text::String {
    switch (_value) {
    case Aes128GcmSha256:
        return "TLS_AES_128_GCM_SHA256"_el;
    case Aes256GcmSha384:
        return "TLS_AES_256_GCM_SHA384"_el;
    case ChaCha20Poly1305Sha256:
        return "TLS_CHACHA20_POLY1305_SHA256"_el;
    }
    return {};
}

auto TlsCipherSuite::fromRawValue(const uint16_t value) noexcept -> std::optional<TlsCipherSuite> {
    switch (value) {
    case Aes128GcmSha256:
        return TlsCipherSuite{Aes128GcmSha256};
    case Aes256GcmSha384:
        return TlsCipherSuite{Aes256GcmSha384};
    case ChaCha20Poly1305Sha256:
        return TlsCipherSuite{ChaCha20Poly1305Sha256};
    default:
        return std::nullopt;
    }
}

auto TlsCipherSuite::fromRawValueOrThrow(const uint16_t value) -> TlsCipherSuite {
    if (const auto result = fromRawValue(value); result.has_value()) {
        return result.value();
    }
    throw err::ParseError{"Unsupported TLS 1.3 cipher suite."_el};
}

auto TlsCipherSuite::all() noexcept -> std::span<const TlsCipherSuite> {
    static constexpr auto suites = std::array<TlsCipherSuite, 3>{
        Aes256GcmSha384,
        ChaCha20Poly1305Sha256,
        Aes128GcmSha256,
    };
    return suites;
}

}
