// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "AesCbcEncryptorData.hpp"

#include "../../../err/LogicFailure.hpp"
#include "../../../text/Literals.hpp"

namespace erbsland::cryptology::impl {

using namespace mem;
using namespace text::literals;

AesCbcEncryptorData::AesCbcEncryptorData(
    const SymmetricEncryptionType type, const ConstByteSpan key, const ConstByteSpan iv) :
    _type{type}, _state{type, key, iv} {
}

AesCbcEncryptorData::~AesCbcEncryptorData() noexcept {
    secureErase();
}

auto AesCbcEncryptorData::type() const noexcept -> SymmetricEncryptionType {
    return _type;
}

void AesCbcEncryptorData::addAuthenticatedData(ConstByteSpan) {
    throw err::LogicFailure{"AES-CBC does not support authenticated data."_el};
}

auto AesCbcEncryptorData::encrypt(const ConstByteSpan data) -> ByteBlock {
    return _state.encrypt(data);
}

auto AesCbcEncryptorData::finalize() -> ByteBlock {
    return _state.finalizeEncryption();
}

auto AesCbcEncryptorData::tag() const -> SymmetricTag {
    throw err::LogicFailure{"AES-CBC does not produce an authentication tag."_el};
}

void AesCbcEncryptorData::secureErase() noexcept {
    _state.secureErase();
    _type = {};
}

}
