// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "AesGcmDecryptorData.hpp"

#include "../../../err/LogicError.hpp"
#include "../../../mem/ByteBlock.hpp"
#include "../../../text/Literals.hpp"
#include "../../CryptologyError.hpp"

namespace erbsland::cryptology::impl {

using namespace text::literals;

AesGcmDecryptorData::AesGcmDecryptorData(
    const SymmetricEncryptionType type, const mem::ConstByteSpan key, const mem::ConstByteSpan nonce) :
    _type{type}, _state{key, nonce} {
}

AesGcmDecryptorData::~AesGcmDecryptorData() noexcept {
    secureErase();
}

auto AesGcmDecryptorData::type() const noexcept -> SymmetricEncryptionType {
    return _type;
}

void AesGcmDecryptorData::addAuthenticatedData(const mem::ConstByteSpan data) {
    _state.addAuthenticatedData(data);
}

auto AesGcmDecryptorData::decrypt(const mem::ConstByteSpan data) -> mem::ByteBlock {
    return _state.transform(data, false);
}

auto AesGcmDecryptorData::finalize(const SymmetricTag &tag) -> mem::ByteBlock {
    _tag = _state.finalizeTag();
    // SP 800-38D, Section 7.2: a tag mismatch yields only a generic authentication failure.
    if (!_tag.isEqualConstTime(tag.span())) {
        throw CryptologyError{"AES-GCM authentication failed."_el};
    }
    return {};
}

auto AesGcmDecryptorData::finalize() -> mem::ByteBlock {
    throw err::LogicError{"AES-GCM decryption requires an authentication tag."_el};
}

void AesGcmDecryptorData::secureErase() noexcept {
    _state.secureErase();
    _tag.secureErase();
    _type = {};
}

}
