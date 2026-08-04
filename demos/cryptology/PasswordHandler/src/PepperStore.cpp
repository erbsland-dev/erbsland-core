// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "PepperRotation.hpp"
#include "PepperStore.hpp"

#include <erbsland/conf/Parser.hpp>
#include <erbsland/conf/Value.hpp>
#include <erbsland/conf/ValueType.hpp>
#include <erbsland/core/ApplicationError.hpp>
#include <erbsland/core/ApplicationErrorContext.hpp>
#include <erbsland/cryptology/PasswordHashKey.hpp>
#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/path/PathContent.hpp>
#include <erbsland/path/PathReadTextOptions.hpp>
#include <erbsland/stream/InputStreamSettings.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unit/ByteLength.hpp>
#include <erbsland/util/List.hpp>

#include <utility>

namespace demo {

using namespace el::text::literals;

PepperStore::PepperStore(el::Path path) : _path{std::move(path)} {
}

[[noreturn]] void PepperStore::throwInvalidPepper(const el::String &description) const {
    auto context = el::core::ApplicationErrorContext{"Invalid pepper file"_el, description};
    context.setSourcePath(_path.toString());
    throw el::ApplicationError{context};
}

auto PepperStore::readKeyBytes(const el::conf::ValuePtr &section) const -> el::ByteBlock {
    el::ByteBlock bytes;
    try {
        bytes = section->getBytesOrThrow("Key"_el);
    } catch (const el::Exception &) {
        throwInvalidPepper("Every key entry requires an ELCL byte value named Key."_el);
    }
    if (bytes.length() != el::ByteLength{32U}) {
        bytes.secureErase();
        throwInvalidPepper("Every password-hash key must contain exactly 32 bytes."_el);
    }
    bytes.markAsSensitive();
    return bytes;
}

/// Load the active and fallback peppers from a strict ELCL document.
///
/// Every byte value is marked as sensitive at its shared allocation. The active key has a public identifier. Fallback
/// keys may be identified or, for legacy records, contain one unnamed key.
auto PepperStore::loadHasher() const -> el::PasswordHasher {
    el::conf::DocumentPtr document;
    auto source = el::String{};
    try {
        auto readOptions = el::path::PathReadTextOptions{};
        readOptions.setSensitive(true);
        source = _path.content().readTextOrThrow(readOptions);
        document = el::conf::Parser{}.parseTextOrThrow(source);
    } catch (const el::Exception &) {
        throwInvalidPepper("The file is not a valid ELCL document."_el);
    }
    if (document->size() != 1U || !document->hasValue("Pepper"_el)) {
        throwInvalidPepper("The document must contain only one Pepper section."_el);
    }
    const auto pepper = document->valueOrThrow("Pepper"_el);
    const auto fallbackValue = pepper->value("Fallback"_el);
    const auto expectedSize = fallbackValue == nullptr ? 2U : 3U;
    if (pepper->size() != expectedSize || !pepper->hasValue("Identifier"_el) || !pepper->hasValue("Key"_el)) {
        throwInvalidPepper("Pepper must contain Identifier and Key, plus an optional Fallback section list."_el);
    }

    el::String activeIdentifier;
    try {
        activeIdentifier = pepper->getTextOrThrow("Identifier"_el);
    } catch (const el::Exception &) {
        throwInvalidPepper("The active key Identifier must be text."_el);
    }
    if (activeIdentifier.isEmpty()) {
        throwInvalidPepper("The active key Identifier must not be empty."_el);
    }
    auto activeKey = el::PasswordHashKey::identified(activeIdentifier, readKeyBytes(pepper));
    auto fallbackKeys = el::List<el::PasswordHashKey>{};

    if (fallbackValue != nullptr) {
        if (fallbackValue->type() != el::conf::ValueType::SectionList) {
            throwInvalidPepper("Pepper.Fallback must be a section list."_el);
        }
        for (const auto &entry : *fallbackValue) {
            const auto hasIdentifier = entry->hasValue("Identifier"_el);
            const auto expectedEntrySize = hasIdentifier ? 2U : 1U;
            if (entry->size() != expectedEntrySize || !entry->hasValue("Key"_el)) {
                throwInvalidPepper("Fallback entries contain only an optional Identifier and a required Key."_el);
            }
            const auto keyBytes = readKeyBytes(entry);
            if (hasIdentifier) {
                el::String identifier;
                try {
                    identifier = entry->getTextOrThrow("Identifier"_el);
                } catch (const el::Exception &) {
                    throwInvalidPepper("A fallback Identifier must be text."_el);
                }
                if (identifier.isEmpty()) {
                    throwInvalidPepper("A fallback Identifier must not be empty."_el);
                }
                fallbackKeys.append(el::PasswordHashKey::identified(identifier, keyBytes));
            } else {
                fallbackKeys.append(el::PasswordHashKey{keyBytes});
            }
        }
    }
    // Let PasswordHasher validate identifier uniqueness and the single unnamed-fallback rule.
    try {
        return buildPasswordHasher(activeKey, fallbackKeys);
    } catch (const el::Exception &) {
        throwInvalidPepper("The active and fallback key identifiers do not form a valid rotation set."_el);
    }
}

}
