// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "SymmetricEncryptor.hpp"

#include "../impl/symmetric/SymmetricDataFactory.hpp"
#include "../impl/symmetric/SymmetricEncryptorData.hpp"

#include "../../err/LogicFailure.hpp"
#include "../../err/ParameterFailure.hpp"
#include "../../err/RuntimeError.hpp"
#include "../../mem/ByteBlock.hpp"
#include "../../text/Literals.hpp"

#include <utility>

namespace erbsland::cryptology {

using namespace err;
using namespace mem;
using namespace text::literals;

SymmetricEncryptor::SymmetricEncryptor(
    const SymmetricEncryptionType type, const SymmetricKey &key, const SymmetricNonce &nonce) {
    validateCommon(type, key);
    validateNonce(type, nonce);
    _data = impl::createSymmetricEncryptorData(type, key, nonce);
    _state = State::Active;
}

SymmetricEncryptor::SymmetricEncryptor(
    const SymmetricEncryptionType type, const SymmetricKey &key, const SymmetricIv &iv) {
    validateCommon(type, key);
    validateIv(type, iv);
    _data = impl::createSymmetricEncryptorData(type, key, iv);
    _state = State::Active;
}

SymmetricEncryptor::SymmetricEncryptor(std::unique_ptr<impl::SymmetricEncryptorData> data) noexcept :
    _data{std::move(data)}, _state{_data != nullptr ? State::Active : State::Empty} {
}

SymmetricEncryptor::~SymmetricEncryptor() {
    secureErase();
}

SymmetricEncryptor::SymmetricEncryptor(SymmetricEncryptor &&other) noexcept :
    _data{std::move(other._data)}, _state{std::exchange(other._state, State::Empty)} {
}

auto SymmetricEncryptor::operator=(SymmetricEncryptor &&other) noexcept -> SymmetricEncryptor & {
    if (this != &other) {
        secureErase();
        _data = std::move(other._data);
        _state = std::exchange(other._state, State::Empty);
    }
    return *this;
}

void SymmetricEncryptor::secureErase() noexcept {
    if (_data != nullptr) {
        _data->secureErase();
        _data.reset();
    }
    _state = State::Empty;
}

void SymmetricEncryptor::addAuthenticatedData(const ConstByteSpan data) {
    requireUsable();
    if (!type().isAead()) {
        throw LogicFailure{"Authenticated data requires an AEAD encryption type."_el};
    }
    if (_state != State::Active) {
        throw LogicFailure{"Authenticated data must be added before payload encryption."_el};
    }
    try {
        _data->addAuthenticatedData(data);
    } catch (const RuntimeError &) {
        _state = State::Failed;
        throw;
    }
}

void SymmetricEncryptor::addAuthenticatedData(const ByteBlock &data) {
    addAuthenticatedData(data.span());
}

auto SymmetricEncryptor::encrypt(const ConstByteSpan data) -> ByteBlock {
    requireUsable();
    if (_state == State::Finalized) {
        throw LogicFailure{"Cannot encrypt data after finalization."_el};
    }
    try {
        auto result = _data->encrypt(data);
        if (!data.empty()) {
            _state = State::Payload;
        }
        return result;
    } catch (const RuntimeError &) {
        _state = State::Failed;
        throw;
    }
}

auto SymmetricEncryptor::encrypt(const ByteBlock &data) -> ByteBlock {
    return encrypt(data.span());
}

auto SymmetricEncryptor::finalize() -> ByteBlock {
    requireUsable();
    if (_state == State::Finalized) {
        throw LogicFailure{"Cannot finalize encryption more than once."_el};
    }
    try {
        auto result = _data->finalize();
        _state = State::Finalized;
        return result;
    } catch (const RuntimeError &) {
        _state = State::Failed;
        throw;
    }
}

auto SymmetricEncryptor::tag() const -> SymmetricTag {
    requireUsable();
    if (!type().isAead()) {
        throw LogicFailure{"An authentication tag is available only for AEAD encryption."_el};
    }
    if (_state != State::Finalized) {
        throw LogicFailure{"The authentication tag is available only after finalization."_el};
    }
    return _data->tag();
}

auto SymmetricEncryptor::type() const -> SymmetricEncryptionType {
    if (isEmpty()) {
        throw LogicFailure{"Cannot access an empty symmetric encryptor."_el};
    }
    return _data->type();
}

void SymmetricEncryptor::validateCommon(const SymmetricEncryptionType type, const SymmetricKey &key) {
    if (!type.isValid()) {
        throw ParameterFailure{"A valid symmetric encryption type is required."_el, "type"_el};
    }
    if (key.byteLength() != type.keyLength()) {
        throw ParameterFailure{"The symmetric key has the wrong length."_el, "key"_el};
    }
}

void SymmetricEncryptor::validateNonce(const SymmetricEncryptionType type, const SymmetricNonce &nonce) {
    if (!type.isAead()) {
        throw ParameterFailure{"This encryption type requires an initialization vector."_el, "type"_el};
    }
    if (nonce.byteLength() != type.nonceLength()) {
        throw ParameterFailure{"The symmetric nonce has the wrong length."_el, "nonce"_el};
    }
}

void SymmetricEncryptor::validateIv(const SymmetricEncryptionType type, const SymmetricIv &iv) {
    if (!type.requiresIv()) {
        throw ParameterFailure{"This encryption type requires a nonce."_el, "type"_el};
    }
    if (iv.byteLength() != type.ivLength()) {
        throw ParameterFailure{"The symmetric initialization vector has the wrong length."_el, "iv"_el};
    }
}

void SymmetricEncryptor::requireUsable() const {
    if (isEmpty()) {
        throw LogicFailure{"Cannot use an empty symmetric encryptor."_el};
    }
    if (_state == State::Failed) {
        throw LogicFailure{"Cannot reuse a symmetric encryptor after a backend failure."_el};
    }
}

}
