// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "AesCbcDecryptorData.hpp"

#include "../../../err/LogicFailure.hpp"
#include "../../../text/Literals.hpp"

namespace erbsland::cryptology::impl {

using namespace mem;
using namespace text::literals;

AesCbcDecryptorData::AesCbcDecryptorData(
    const SymmetricEncryptionType type, const ConstByteSpan key, const ConstByteSpan iv) :
    _type{type}, _state{type, key, iv} {
}

AesCbcDecryptorData::~AesCbcDecryptorData() noexcept {
    secureErase();
}

auto AesCbcDecryptorData::type() const noexcept -> SymmetricEncryptionType {
    return _type;
}

void AesCbcDecryptorData::addAuthenticatedData(ConstByteSpan) {
    throw err::LogicFailure{"AES-CBC does not support authenticated data."_el};
}

auto AesCbcDecryptorData::decrypt(const ConstByteSpan data) -> ByteBlock {
    return _state.decrypt(data);
}

auto AesCbcDecryptorData::finalize(const SymmetricTag &) -> ByteBlock {
    throw err::LogicFailure{"AES-CBC decryption does not accept an authentication tag."_el};
}

auto AesCbcDecryptorData::finalize() -> ByteBlock {
    return _state.finalizeDecryption();
}

void AesCbcDecryptorData::secureErase() noexcept {
    _state.secureErase();
    _type = {};
}

}
