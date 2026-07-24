// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "PepperStore.hpp"
#include "StorageFile.hpp"

#include <erbsland/core/Application.hpp>
#include <erbsland/cryptology/PasswordHashKey.hpp>
#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/path/PathCollisionMode.hpp>
#include <erbsland/random/Random.hpp>
#include <erbsland/text/CharSet.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringEditor.hpp>
#include <erbsland/text/StringFormat.hpp>
#include <erbsland/unit/ByteLength.hpp>
#include <erbsland/unit/CpLength.hpp>

namespace demo {

using namespace el::text::literals;

/// Generate and persist the initial identified password-hash pepper.
///
/// The key identifier is public, random, and filename-safe. The 32 random bytes are generated directly in protected
/// storage. The ELCL file uses a native byte literal and collision-stop creation so an existing key is never silently
/// replaced.
void PepperStore::create(const el::Path &path) {
    constexpr auto pepperLength = el::ByteLength{32U};
    static const auto identifierCharacters = el::CharSet{"abcdefghijklmnopqrstuvwxyz0123456789"_el};
    const auto identifier = el::String::fromJoined(
        {"key-"_el, el::application().secureRandom().buildString(el::CpLength{16U}, identifierCharacters)});

    const auto protectedBytes = el::application().secureRandom().buildByteBlock(pepperLength);
    const auto activeKey = el::PasswordHashKey::identified(identifier, protectedBytes);
    auto output = el::StringEditor{"@version: \"1.0\"\n\n[Pepper]\n"_el};
    appendElclText(output, "Identifier"_el, identifier);
    output.append(el::StringFormat{"Key: <{}>\n"_el}.build(protectedBytes));
    static_cast<void>(activeKey);
    writeStorageFile(path, el::String{output}, el::PathCollisionMode::Stop);
}

}
