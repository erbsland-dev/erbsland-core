// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "AesGcmEncryptorData.hpp"

#include "../../../mem/ByteBlock.hpp"

namespace erbsland::cryptology::impl {

AesGcmEncryptorData::AesGcmEncryptorData(
    const SymmetricEncryptionType type, const mem::ConstByteSpan key, const mem::ConstByteSpan nonce) :
    _type{type}, _state{key, nonce} {
}

AesGcmEncryptorData::~AesGcmEncryptorData() noexcept {
    secureErase();
}

auto AesGcmEncryptorData::type() const noexcept -> SymmetricEncryptionType {
    return _type;
}

void AesGcmEncryptorData::addAuthenticatedData(const mem::ConstByteSpan data) {
    _state.addAuthenticatedData(data);
}

auto AesGcmEncryptorData::encrypt(const mem::ConstByteSpan data) -> mem::ByteBlock {
    return _state.transform(data, true);
}

auto AesGcmEncryptorData::finalize() -> mem::ByteBlock {
    _tag = _state.finalizeTag();
    return {};
}

auto AesGcmEncryptorData::tag() const -> SymmetricTag {
    return SymmetricTag{mem::ByteBlock{_tag}};
}

void AesGcmEncryptorData::secureErase() noexcept {
    _state.secureErase();
    _tag.secureErase();
    _type = {};
}

}
