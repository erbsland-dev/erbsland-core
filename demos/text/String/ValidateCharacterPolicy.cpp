// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Combine encoding checks with `containsOnly()` and `containsOneOf()` for character-level validation.
///
/// Validate externally supplied text before applying tolerant decoded-character operations.
void validateCharacterPolicy() {
    static const auto userNameChars = el::CharSet::fromPattern("-_a-zA-Z0-9"_el);
    static const auto forbiddenChars = el::CharSet{" \t\r\n"_el};

    const auto userNames = el::StringList{
        "orbite-07"_el,
        "module solaire"_el,
        "équipe-science"_el,
    };

    const auto yesNo = el::BooleanFormat::yesNo();
    userNames.forEach([&](const el::String &userName) -> void {
        const auto isAccepted =
            userName.isValidUtf8() && userName.containsOnly(userNameChars) && !userName.containsOneOf(forbiddenChars);
        el::io::printLine(userName, " -> "_el, yesNo, isAccepted);
    });
}

}
