// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HostName.hpp"

#include "../err/ParseError.hpp"
#include "../text/StringConverter.hpp"

namespace erbsland::network {

using namespace unit;
using namespace text;
using namespace text::literals;

auto HostName::fromString(const String &text) noexcept -> std::optional<HostName> {
    static const auto invalidCharacters = []() -> CharSet {
        auto result = CharSet{":%[]/\\"_el};
        result.add(CharSet::from(UnicodeCategoryGroup::Other));
        result.add(CharSet::from(UnicodeCategoryGroup::Separator));
        return result;
    }();
    try {
        if (text.isEmpty() || !text.isValidUtf8() || text.length() > ByteLength{1024U} ||
            text.containsOneOf(invalidCharacters)) {
            return std::nullopt;
        }
        return HostName{text};
    } catch (const err::Exception &) {
        return std::nullopt;
    }
}

auto HostName::fromStringOrThrow(const String &text) -> HostName {
    if (const auto result = fromString(text); result.has_value()) {
        return *result;
    }
    throw err::ParseError{"The text is not a valid platform-resolvable host name."};
}

}
