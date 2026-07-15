// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PathNameTools.hpp"

#include "CharactersSets.hpp"

#include "../../text/CharSet.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringBuilder.hpp"
#include "../../unit/ByteLength.hpp"
#include "../../unit/ByteRange.hpp"

namespace erbsland::path::impl {

using namespace text::literals;

[[nodiscard]] auto suffixSearchStart(const text::StringView &name) noexcept -> unit::ByteIndex {
    if (name.isEmpty()) {
        return unit::ByteIndex::noIndex();
    }
    // skip initial dots, as these are not considered valid suffixes
    unit::ByteIndex result;
    auto character = name.charAt(result);
    while (character == U'.') {
        name.advance(result);
        character = name.charAt(result);
    }
    if (character.isEndOfData()) {
        return unit::ByteIndex::noIndex(); // the name has just dots
    }
    return result;
}

auto firstSuffixPosition(const text::StringView &name) noexcept -> unit::ByteIndex {
    // `suffixSearchStart` never points to a dot - therefore, no special handling required.
    // if `suffixSearchStart` returns no-index, `findFirstOf` also returns no-index.
    return name.findFirstOf(dotCharacters(), suffixSearchStart(name));
}

auto lastSuffixPosition(const text::StringView &name) noexcept -> unit::ByteIndex {
    // for reverse search, get the start first.
    const auto start = suffixSearchStart(name);
    if (start.isNoIndex()) { // only dots or empty.
        return start;
    }
    const auto position = name.findLastOf(dotCharacters());
    if (position.isNoIndex() || position < start) { // no dot found, or the dot is part of the initial dot sequence.
        return unit::ByteIndex::noIndex();
    }
    return position;
}

auto lastSuffix(const text::StringView &name) noexcept -> text::StringView {
    // if `lastSuffixPosition` returns no-index, `name.slice` also returns no-index.
    return name.slice({lastSuffixPosition(name), unit::ByteLength::infinite()});
}

auto suffixes(const text::StringView &name) noexcept -> text::StringView {
    // if `firstSuffixPosition` returns no-index, `name.slice` also returns no-index.
    return name.slice({firstSuffixPosition(name), unit::ByteLength::infinite()});
}

auto stem(const text::StringView &name) noexcept -> text::StringView {
    const auto position = firstSuffixPosition(name);
    if (position.isNoIndex()) {
        return name;
    }
    return name.slice(text::StringSide::Front, position.distanceFromZero());
}

auto normalizedSuffixReplacement(const text::StringView &replacement) -> text::StringView {
    if (replacement.isEmpty() || replacement.startsWith("."_el)) {
        return replacement;
    }
    return "."_els.append(replacement);
}

}
