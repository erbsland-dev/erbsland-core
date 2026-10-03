// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "AesGcmDecryptorData.hpp"

#include "../../../err/LogicFailure.hpp"
#include "../../../mem/ByteBlock.hpp"
#include "../../../text/Literals.hpp"
#include "../../CryptologyError.hpp"

namespace erbsland::cryptology::impl {

using namespace mem;
using namespace text::literals;

AesGcmDecryptorData::AesGcmDecryptorData(
    const SymmetricEncryptionType type, const ConstByteSpan key, const ConstByteSpan nonce) :
    _type{type}, _state{key, nonce} {
}

AesGcmDecryptorData::~AesGcmDecryptorData() noexcept {
    secureErase();
}

auto AesGcmDecryptorData::type() const noexcept -> SymmetricEncryptionType {
    return _type;
}

void AesGcmDecryptorData::addAuthenticatedData(const ConstByteSpan data) {
    _state.addAuthenticatedData(data);
}

auto AesGcmDecryptorData::decrypt(const ConstByteSpan data) -> ByteBlock {
    return _state.transform(data, false);
}

auto AesGcmDecryptorData::finalize(const SymmetricTag &tag) -> ByteBlock {
    _tag = _state.finalizeTag();
    // SP 800-38D, Section 7.2: a tag mismatch yields only a generic authentication failure.
    if (!_tag.isEqualConstTime(tag.span())) {
        throw CryptologyError{"AES-GCM authentication failed."_el};
    }
    return {};
}

auto AesGcmDecryptorData::finalize() -> ByteBlock {
    throw err::LogicFailure{"AES-GCM decryption requires an authentication tag."_el};
}

void AesGcmDecryptorData::secureErase() noexcept {
    _state.secureErase();
    _tag.secureErase();
    _type = {};
}

}
