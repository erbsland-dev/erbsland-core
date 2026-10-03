// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PathNameTools.hpp"

#include "CharactersSets.hpp"

#include "../../text/CharSet.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringEditor.hpp"
#include "../../text/StringJoins.hpp"
#include "../../unit/ByteCount.hpp"
#include "../../unit/ByteRange.hpp"

namespace erbsland::path::impl {

using namespace unit;
using namespace text::literals;
using text::String;
using text::StringSide;

[[nodiscard]] auto suffixSearchStart(const String &name) noexcept -> ByteIndex {
    if (name.isEmpty()) {
        return ByteIndex::noIndex();
    }
    // skip initial dots, as these are not considered valid suffixes
    ByteIndex result;
    auto character = name.charAt(result);
    while (character == U'.') {
        name.advance(result);
        character = name.charAt(result);
    }
    if (character.isEndOfData()) {
        return ByteIndex::noIndex(); // the name has just dots
    }
    return result;
}

auto firstSuffixPosition(const String &name) noexcept -> ByteIndex {
    // `suffixSearchStart` never points to a dot - therefore, no special handling required.
    // if `suffixSearchStart` returns no-index, `findFirstOf` also returns no-index.
    return name.findFirstOf(dotCharacters(), suffixSearchStart(name));
}

auto lastSuffixPosition(const String &name) noexcept -> ByteIndex {
    // for reverse search, get the start first.
    const auto start = suffixSearchStart(name);
    if (start.isNoIndex()) { // only dots or empty.
        return start;
    }
    const auto position = name.findLastOf(dotCharacters());
    if (position.isNoIndex() || position < start) { // no dot found, or the dot is part of the initial dot sequence.
        return ByteIndex::noIndex();
    }
    return position;
}

auto lastSuffix(const String &name) noexcept -> String {
    // if `lastSuffixPosition` returns no-index, `name.slice` also returns no-index.
    return name.slice({lastSuffixPosition(name), ByteCount::infinite()});
}

auto suffixes(const String &name) noexcept -> String {
    // if `firstSuffixPosition` returns no-index, `name.slice` also returns no-index.
    return name.slice({firstSuffixPosition(name), ByteCount::infinite()});
}

auto stem(const String &name) noexcept -> String {
    const auto position = firstSuffixPosition(name);
    if (position.isNoIndex()) {
        return name;
    }
    return name.slice(StringSide::Front, position.distanceFromZero());
}

auto normalizedSuffixReplacement(const String &replacement) -> String {
    if (replacement.isEmpty() || replacement.startsWith("."_el)) {
        return replacement;
    }
    return text::join("."_el, replacement);
}

}
