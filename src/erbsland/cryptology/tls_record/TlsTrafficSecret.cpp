// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TlsTrafficSecret.hpp"

#include "../impl/SecureEraseGuard.hpp"

#include "../../err/OutOfRangeError.hpp"
#include "../../err/ParameterFailure.hpp"
#include "../../mem/ByteBlock.hpp"
#include "../../text/Literals.hpp"

#include <utility>

namespace erbsland::cryptology {

using namespace mem;
using namespace text::literals;

TlsTrafficSecret::TlsTrafficSecret(const HashAlgorithm algorithm, const ConstByteSpan data) :
    _hashAlgorithm{algorithm}, _data{data} {
}

TlsTrafficSecret::~TlsTrafficSecret() {
    secureErase();
}

void TlsTrafficSecret::secureErase() noexcept {
    // RFC 8446 section 7.2: old traffic secrets are deleted after their successor and keys have been installed.
    _data.secureErase();
}

auto TlsTrafficSecret::fromBytes(const HashAlgorithm algorithm, const ConstByteSpan data) -> TlsTrafficSecret {
    validate(algorithm, data);
    // Protect the borrowed traffic secret immediately. The caller remains responsible for its source allocation.
    return TlsTrafficSecret{algorithm, data};
}

auto TlsTrafficSecret::fromBytes(const HashAlgorithm algorithm, ByteBlock &&data) -> TlsTrafficSecret {
    validate(algorithm, data.span());
    data.markAsSensitive();
    // The owning plaintext secret remains guarded while ProtectedByteBlock creates its authenticated envelope.
    const auto eraseGuard = impl::SecureEraseGuard{data};
    return TlsTrafficSecret{algorithm, data.span()};
}

void TlsTrafficSecret::validate(const HashAlgorithm algorithm, const ConstByteSpan data) {
    if (algorithm != HashAlgorithm::Sha2_256 && algorithm != HashAlgorithm::Sha2_384) {
        throw err::ParameterFailure{"A TLS traffic secret requires SHA-256 or SHA-384."_el, "algorithm"_el};
    }
    if (data.size() != algorithm.digestSize().toSizeT()) {
        throw err::OutOfRangeError{"A TLS traffic secret must match the digest size."_el};
    }
}

}
