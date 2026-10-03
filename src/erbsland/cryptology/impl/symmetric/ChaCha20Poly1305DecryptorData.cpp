// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ChaCha20Poly1305DecryptorData.hpp"

#include "../../../err/LogicFailure.hpp"
#include "../../../mem/ByteBlock.hpp"
#include "../../../text/Literals.hpp"
#include "../../CryptologyError.hpp"

namespace erbsland::cryptology::impl {

using namespace mem;
using namespace text::literals;

ChaCha20Poly1305DecryptorData::ChaCha20Poly1305DecryptorData(const ConstByteSpan key, const ConstByteSpan nonce) :
    _state{key, nonce} {
}

ChaCha20Poly1305DecryptorData::~ChaCha20Poly1305DecryptorData() noexcept {
    secureErase();
}

auto ChaCha20Poly1305DecryptorData::type() const noexcept -> SymmetricEncryptionType {
    return SymmetricEncryptionType::ChaCha20Poly1305;
}

void ChaCha20Poly1305DecryptorData::addAuthenticatedData(const ConstByteSpan data) {
    _state.addAuthenticatedData(data);
}

auto ChaCha20Poly1305DecryptorData::decrypt(const ConstByteSpan data) -> ByteBlock {
    return _state.transform(data, false);
}

auto ChaCha20Poly1305DecryptorData::finalize(const SymmetricTag &tag) -> ByteBlock {
    _actualTag = _state.finalizeTag();
    // RFC 8439, Section 2.8: report no detail beyond a generic authentication failure.
    if (!_actualTag.isEqualConstTime(tag.span())) {
        throw CryptologyError{"ChaCha20-Poly1305 authentication failed."_el};
    }
    return {};
}

auto ChaCha20Poly1305DecryptorData::finalize() -> ByteBlock {
    throw err::LogicFailure{"ChaCha20-Poly1305 decryption requires an authentication tag."_el};
}

void ChaCha20Poly1305DecryptorData::secureErase() noexcept {
    _state.secureErase();
    _actualTag.secureErase();
}

}
