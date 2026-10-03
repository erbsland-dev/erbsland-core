// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PepperStore.hpp"

#include "StorageFile.hpp"

#include <erbsland/Application.hpp>
#include <erbsland/ByteBlock.hpp>
#include <erbsland/ByteCount.hpp>
#include <erbsland/CharSet.hpp>
#include <erbsland/CpCount.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/PathCollisionMode.hpp>
#include <erbsland/Random.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/StringJoins.hpp>

namespace demo {

using namespace el::text::literals;

/// Generate and persist the initial identified password-hash pepper.
///
/// The key identifier is public, random, and filename-safe. The 32 random bytes are generated directly in protected
/// storage. The ELCL file uses a native byte literal and collision-stop creation so an existing key is never silently
/// replaced.
void PepperStore::create() const {
    constexpr auto pepperLength = el::ByteCount{32U};
    static const auto identifierCharacters = el::CharSet{"abcdefghijklmnopqrstuvwxyz0123456789"_el};
    const auto identifier =
        el::join("key-"_el, el::application().secureRandom().buildString(el::CpCount{16U}, identifierCharacters));

    auto protectedBytes = el::application().secureRandom().buildByteBlock(pepperLength);
    protectedBytes.markAsSensitive();
    auto output = el::StringEditor{"@version: \"1.0\"\n\n[Pepper]\n"_el};
    appendElclText(output, "Identifier"_el, identifier);
    output.append(el::StringFormat{"Key: <{}>\n"_el}.build(protectedBytes));
    writeStorageFile(_path, output, el::PathCollisionMode::Stop);
}

}
