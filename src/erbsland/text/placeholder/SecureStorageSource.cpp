// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "SecureStorageSource.hpp"

#include "ReplacerError.hpp"

#include "../impl/UnsafeU8StringBuffer.hpp"
#include "../Literals.hpp"
#include "../u8/U8StringConstIterator.hpp"

#include "../../cryptology/SecureStorageError.hpp"

namespace erbsland::text::placeholder {

using namespace literals;

auto SecureStorageSource::sourceNames() const -> StringList {
    return StringList{"secure"_el};
}

auto SecureStorageSource::validate(const String &sourceName, const String &parameter) -> bool {
    return sourceName == "secure"_el && cryptology::SecureStorage::isValidName(parameter);
}

auto SecureStorageSource::resolve(const String &sourceName, const String &parameter) -> String {
    if (!validate(sourceName, parameter)) {
        throw ReplacerError{ReplacerErrorCategory::Syntax, "Invalid secure-storage placeholder name."_el};
    }
    auto stored = std::optional<cryptology::ProtectedByteBlock>{};
    try {
        stored = _storage.read(parameter);
    } catch (const cryptology::SecureStorageError &) {
        throw ReplacerError{ReplacerErrorCategory::Access, "Secure-storage retrieval failed."_el};
    }
    if (!stored.has_value()) {
        throw ReplacerError{ReplacerErrorCategory::ValueNotFound, "Secure-storage item does not exist."_el};
    }
    auto result = String{};
    stored->withUnprotectedData([&](const mem::ConstByteSpan plaintext) -> void {
        result = text::impl::UnsafeU8StringBuffer{plaintext, true}.takeString();
    });
    if (!result.isEncodingValid()) {
        throw ReplacerError{ReplacerErrorCategory::Validation, "Secure-storage item is not valid UTF-8 text."_el};
    }
    for (const auto character : result) {
        const auto codePoint = character.toRawValue();
        if (character.isControl() && codePoint != 0x09U && codePoint != 0x0aU && codePoint != 0x0dU) {
            throw ReplacerError{ReplacerErrorCategory::Validation, "Secure-storage item contains binary data."_el};
        }
    }
    return result;
}

}
