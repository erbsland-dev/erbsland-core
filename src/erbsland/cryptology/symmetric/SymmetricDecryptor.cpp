// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "SymmetricDecryptor.hpp"

#include "../impl/symmetric/SymmetricDataFactory.hpp"
#include "../impl/symmetric/SymmetricDecryptorData.hpp"

#include "../../err/LogicError.hpp"
#include "../../err/ParameterError.hpp"
#include "../../mem/ByteBlock.hpp"
#include "../../text/Literals.hpp"

#include <utility>

namespace erbsland::cryptology {

using namespace text::literals;

SymmetricDecryptor::SymmetricDecryptor(
    const SymmetricEncryptionType type, const SymmetricKey &key, const SymmetricNonce &nonce) {
    validateCommon(type, key);
    validateNonce(type, nonce);
    _data = impl::createSymmetricDecryptorData(type, key, nonce);
    _state = State::Active;
}

SymmetricDecryptor::SymmetricDecryptor(
    const SymmetricEncryptionType type, const SymmetricKey &key, const SymmetricIv &iv) {
    validateCommon(type, key);
    validateIv(type, iv);
    _data = impl::createSymmetricDecryptorData(type, key, iv);
    _state = State::Active;
}

SymmetricDecryptor::SymmetricDecryptor(std::unique_ptr<impl::SymmetricDecryptorData> data) noexcept :
    _data{std::move(data)}, _state{_data != nullptr ? State::Active : State::Empty} {
}

SymmetricDecryptor::~SymmetricDecryptor() {
    secureErase();
}

SymmetricDecryptor::SymmetricDecryptor(SymmetricDecryptor &&other) noexcept :
    _data{std::move(other._data)}, _state{std::exchange(other._state, State::Empty)} {
}

auto SymmetricDecryptor::operator=(SymmetricDecryptor &&other) noexcept -> SymmetricDecryptor & {
    if (this != &other) {
        secureErase();
        _data = std::move(other._data);
        _state = std::exchange(other._state, State::Empty);
    }
    return *this;
}

void SymmetricDecryptor::secureErase() noexcept {
    if (_data != nullptr) {
        _data->secureErase();
        _data.reset();
    }
    _state = State::Empty;
}

void SymmetricDecryptor::addAuthenticatedData(const mem::ConstByteSpan data) {
    requireUsable();
    if (!type().isAead()) {
        throw err::LogicError{"Authenticated data requires an AEAD encryption type."_el};
    }
    if (_state != State::Active) {
        throw err::LogicError{"Authenticated data must be added before payload decryption."_el};
    }
    try {
        _data->addAuthenticatedData(data);
    } catch (...) {
        _state = State::Failed;
        throw;
    }
}

void SymmetricDecryptor::addAuthenticatedData(const mem::ByteBlock &data) {
    addAuthenticatedData(data.span());
}

auto SymmetricDecryptor::decrypt(const mem::ConstByteSpan data) -> mem::ByteBlock {
    requireUsable();
    if (_state == State::Finalized) {
        throw err::LogicError{"Cannot decrypt data after finalization."_el};
    }
    try {
        auto result = markSensitive(_data->decrypt(data));
        if (!data.empty()) {
            _state = State::Payload;
        }
        return result;
    } catch (...) {
        _state = State::Failed;
        throw;
    }
}

auto SymmetricDecryptor::decrypt(const mem::ByteBlock &data) -> mem::ByteBlock {
    return decrypt(data.span());
}

auto SymmetricDecryptor::finalize(const SymmetricTag &tag) -> mem::ByteBlock {
    requireUsable();
    if (!type().isAead()) {
        throw err::LogicError{"Tag finalization requires an AEAD encryption type."_el};
    }
    if (_state == State::Finalized) {
        throw err::LogicError{"Cannot finalize decryption more than once."_el};
    }
    if (tag.byteLength() != type().tagLength()) {
        throw err::ParameterError{"The authentication tag has the wrong length."_el, "tag"_el};
    }
    try {
        auto result = markSensitive(_data->finalize(tag));
        _state = State::Finalized;
        return result;
    } catch (...) {
        _state = State::Failed;
        throw;
    }
}

auto SymmetricDecryptor::finalize() -> mem::ByteBlock {
    requireUsable();
    if (type().isAead()) {
        throw err::LogicError{"AEAD decryption must be finalized with an authentication tag."_el};
    }
    if (_state == State::Finalized) {
        throw err::LogicError{"Cannot finalize decryption more than once."_el};
    }
    try {
        auto result = markSensitive(_data->finalize());
        _state = State::Finalized;
        return result;
    } catch (...) {
        _state = State::Failed;
        throw;
    }
}

auto SymmetricDecryptor::type() const -> SymmetricEncryptionType {
    if (isEmpty()) {
        throw err::LogicError{"Cannot access an empty symmetric decryptor."_el};
    }
    return _data->type();
}

void SymmetricDecryptor::validateCommon(const SymmetricEncryptionType type, const SymmetricKey &key) {
    if (!type.isValid()) {
        throw err::ParameterError{"A valid symmetric encryption type is required."_el, "type"_el};
    }
    if (key.byteLength() != type.keyLength()) {
        throw err::ParameterError{"The symmetric key has the wrong length."_el, "key"_el};
    }
}

void SymmetricDecryptor::validateNonce(const SymmetricEncryptionType type, const SymmetricNonce &nonce) {
    if (!type.isAead()) {
        throw err::ParameterError{"This encryption type requires an initialization vector."_el, "type"_el};
    }
    if (nonce.byteLength() != type.nonceLength()) {
        throw err::ParameterError{"The symmetric nonce has the wrong length."_el, "nonce"_el};
    }
}

void SymmetricDecryptor::validateIv(const SymmetricEncryptionType type, const SymmetricIv &iv) {
    if (!type.requiresIv()) {
        throw err::ParameterError{"This encryption type requires a nonce."_el, "type"_el};
    }
    if (iv.byteLength() != type.ivLength()) {
        throw err::ParameterError{"The symmetric initialization vector has the wrong length."_el, "iv"_el};
    }
}

auto SymmetricDecryptor::markSensitive(mem::ByteBlock result) noexcept -> mem::ByteBlock {
    result.markAsSensitive();
    return result;
}

void SymmetricDecryptor::requireUsable() const {
    if (isEmpty()) {
        throw err::LogicError{"Cannot use an empty symmetric decryptor."_el};
    }
    if (_state == State::Failed) {
        throw err::LogicError{"Cannot reuse a symmetric decryptor after a backend failure."_el};
    }
}

}
