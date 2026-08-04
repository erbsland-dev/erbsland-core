// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ChaCha20Poly1305EncryptorData.hpp"

#include "../../../mem/ByteBlock.hpp"

namespace erbsland::cryptology::impl {

ChaCha20Poly1305EncryptorData::ChaCha20Poly1305EncryptorData(
    const mem::ConstByteSpan key, const mem::ConstByteSpan nonce) :
    _state{key, nonce} {
}

ChaCha20Poly1305EncryptorData::~ChaCha20Poly1305EncryptorData() noexcept {
    secureErase();
}

auto ChaCha20Poly1305EncryptorData::type() const noexcept -> SymmetricEncryptionType {
    return SymmetricEncryptionType::ChaCha20Poly1305;
}

void ChaCha20Poly1305EncryptorData::addAuthenticatedData(const mem::ConstByteSpan data) {
    _state.addAuthenticatedData(data);
}

auto ChaCha20Poly1305EncryptorData::encrypt(const mem::ConstByteSpan data) -> mem::ByteBlock {
    return _state.transform(data, true);
}

auto ChaCha20Poly1305EncryptorData::finalize() -> mem::ByteBlock {
    _tag = _state.finalizeTag();
    return {};
}

auto ChaCha20Poly1305EncryptorData::tag() const -> SymmetricTag {
    return SymmetricTag{_tag.span()};
}

void ChaCha20Poly1305EncryptorData::secureErase() noexcept {
    _state.secureErase();
    _tag.secureErase();
}

}
