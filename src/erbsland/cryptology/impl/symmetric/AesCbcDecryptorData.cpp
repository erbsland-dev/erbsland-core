// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "AesCbcDecryptorData.hpp"

#include "../../../err/LogicError.hpp"
#include "../../../text/Literals.hpp"

namespace erbsland::cryptology::impl {

using namespace text::literals;

AesCbcDecryptorData::AesCbcDecryptorData(
    const SymmetricEncryptionType type, const mem::ConstByteSpan key, const mem::ConstByteSpan iv) :
    _type{type}, _state{type, key, iv} {
}

AesCbcDecryptorData::~AesCbcDecryptorData() noexcept {
    secureErase();
}

auto AesCbcDecryptorData::type() const noexcept -> SymmetricEncryptionType {
    return _type;
}

void AesCbcDecryptorData::addAuthenticatedData(mem::ConstByteSpan) {
    throw err::LogicError{"AES-CBC does not support authenticated data."_el};
}

auto AesCbcDecryptorData::decrypt(const mem::ConstByteSpan data) -> mem::ByteBlock {
    return _state.decrypt(data);
}

auto AesCbcDecryptorData::finalize(const SymmetricTag &) -> mem::ByteBlock {
    throw err::LogicError{"AES-CBC decryption does not accept an authentication tag."_el};
}

auto AesCbcDecryptorData::finalize() -> mem::ByteBlock {
    return _state.finalizeDecryption();
}

void AesCbcDecryptorData::secureErase() noexcept {
    _state.secureErase();
    _type = {};
}

}
