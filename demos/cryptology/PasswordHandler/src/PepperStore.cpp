// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PepperStore.hpp"

#include "PepperRotation.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/ApplicationErrorContext.hpp>
#include <erbsland/ByteBlock.hpp>
#include <erbsland/ByteCount.hpp>
#include <erbsland/conf/ConfError.hpp>
#include <erbsland/conf/Document.hpp>
#include <erbsland/conf/Parser.hpp>
#include <erbsland/conf/Value.hpp>
#include <erbsland/conf/ValueType.hpp>
#include <erbsland/InputStreamSettings.hpp>
#include <erbsland/List.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/ParameterFailure.hpp>
#include <erbsland/PasswordHasher.hpp>
#include <erbsland/PasswordHashKey.hpp>
#include <erbsland/Path.hpp>
#include <erbsland/PathContent.hpp>
#include <erbsland/PathReadTextOptions.hpp>
#include <erbsland/RuntimeError.hpp>
#include <erbsland/String.hpp>

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
    } catch (const el::conf::ConfError &) {
        throwInvalidPepper("Every key entry requires an ELCL byte value named Key."_el);
    }
    if (bytes.length() != el::ByteCount{32U}) {
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
    } catch (const el::RuntimeError &) {
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
    } catch (const el::conf::ConfError &) {
        throwInvalidPepper("The active key Identifier must be text."_el);
    }
    if (!el::PasswordHashKey::isValidIdentifier(activeIdentifier)) {
        throwInvalidPepper("The active key Identifier must contain 1-32 ASCII letters, digits, '.', '_' or '-'."_el);
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
                } catch (const el::conf::ConfError &) {
                    throwInvalidPepper("A fallback Identifier must be text."_el);
                }
                if (!el::PasswordHashKey::isValidIdentifier(identifier)) {
                    throwInvalidPepper(
                        "A fallback Identifier must contain 1-32 ASCII letters, digits, '.', '_' or '-'."_el);
                }
                fallbackKeys.append(el::PasswordHashKey::identified(identifier, keyBytes));
            } else {
                fallbackKeys.append(el::PasswordHashKey{keyBytes});
            }
        }
    }
    auto identifiers = el::List<el::String>{};
    identifiers.append(activeIdentifier);
    auto hasUnnamed = false;
    for (const auto &key : fallbackKeys) {
        if (const auto identifier = key.identifier(); identifier.has_value()) {
            if (identifiers.contains(*identifier)) {
                throwInvalidPepper("Active and fallback key identifiers must be unique."_el);
            }
            identifiers.append(*identifier);
        } else {
            if (hasUnnamed) {
                throwInvalidPepper("Only one unnamed fallback key is allowed."_el);
            }
            hasUnnamed = true;
        }
    }
    return buildPasswordHasher(activeKey, fallbackKeys);
}

}
