// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BlockAttributes.hpp"

#include "../err/ParseError.hpp"
#include "../text/Literals.hpp"
#include "../text/named_key/Format.hpp"
#include "../text/named_key/Parser.hpp"
#include "../text/StringCharReader.hpp"
#include "../text/StringEditor.hpp"

#include <array>

namespace erbsland::cterm {

using namespace text::literals;

auto BlockAttributes::attributeFlags() noexcept -> const std::array<Flag, 8> & {
    static constexpr auto flags = std::array{
        Bold,
        Dim,
        Italic,
        Underline,
        Blink,
        Reverse,
        Hidden,
        Strikethrough,
    };
    return flags;
}

auto BlockAttributes::attributeFormat() -> const text::named_key::Format & {
    static const auto keys = text::named_key::Format::Keys{{
        {"inherited"_el, cInheritedKey},
        {"bold"_el, BlockAttributes::Bold.value},
        {"dim"_el, BlockAttributes::Dim.value},
        {"italic"_el, BlockAttributes::Italic.value},
        {"underline"_el, BlockAttributes::Underline.value},
        {"blink"_el, BlockAttributes::Blink.value},
        {"reverse"_el, BlockAttributes::Reverse.value},
        {"hidden"_el, BlockAttributes::Hidden.value},
        {"strikethrough"_el, BlockAttributes::Strikethrough.value},
    }};
    static const auto format = text::named_key::Format{}
                                   .setKeys(keys)
                                   .setAllowedKeyPrefixes(text::CharSet{U'+', U'-'})
                                   .setValuesAllowed(false)
                                   .setValueListAllowed(false);
    return format;
}

auto BlockAttributes::toString() const -> text::String {
    if (_specifiedMask == 0) {
        return "inherited"_el;
    }
    auto result = text::StringEditor{};
    for (const auto flag : attributeFlags()) {
        if (!isSpecified(flag)) {
            continue;
        }
        if (!result.isEmpty()) {
            result.append(","_el);
        }
        if (!isEnabled(flag)) {
            result.append("-"_el);
        }
        result.append(attributeFormat().keyName(flag.value));
    }
    return text::String{result};
}

auto BlockAttributes::fromString(const text::String &str, const BlockAttributes defaultValue) -> BlockAttributes {
    try {
        return fromStringOrThrow(str);
    } catch (const err::ParseError &) {
        return defaultValue;
    }
}

auto BlockAttributes::fromStringOrThrow(const text::String &str) -> BlockAttributes {
    if (str.isEmpty()) {
        throw err::ParseError{"A block attribute list must not be empty."_el};
    }

    auto reader = text::StringCharReader{str};
    const auto entries = text::named_key::Parser{reader, attributeFormat()}.readAllEntries();
    if (entries.isEmpty()) {
        throw err::ParseError{"A block attribute list must not be empty."_el};
    }
    const auto first = entries.first();
    if (first.keyIndex() == cInheritedKey) {
        if (entries.count() != unit::ItemCount::one() || !first.prefix().isNoCodePoint()) {
            throw err::ParseError{"The inherited attribute state cannot be combined with other attributes."_el};
        }
        return {};
    }

    auto result = BlockAttributes{};
    for (const auto &entry : entries) {
        if (entry.keyIndex() == cInheritedKey) {
            throw err::ParseError{"The inherited attribute state cannot be combined with other attributes."_el};
        }
        const auto flag = Flag{static_cast<uint8_t>(entry.keyIndex())};
        result.setFlag(flag, entry.prefix() != U'-');
    }
    return result;
}

}
