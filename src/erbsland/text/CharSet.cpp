// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CharSet.hpp"

#include "StringBuilder.hpp"

#include "impl/CharSetFromPattern.hpp"
#include "u16/U16String.hpp"
#include "u32/U32String.hpp"
#include "u8/U8String.hpp"
#include "u8/U8StringConstIterator.hpp"
#include "u8/U8StringView.hpp"

#include <algorithm>

namespace erbsland::text {

CharSet::CharSet(const Char character) {
    add(character);
}

CharSet::CharSet(const U8StringView &characters) {
    for (const auto character : characters) {
        add(character);
    }
}

CharSet::CharSet(const util::Set<Char> &characters) {
    for (const auto character : characters) {
        add(character);
    }
}

CharSet::CharSet(const util::List<Char> &characters) {
    for (const auto character : characters) {
        add(character);
    }
}

CharSet::CharSet(const std::initializer_list<Char> characters) {
    for (const auto character : characters) {
        add(character);
    }
}

auto CharSet::operator==(const CharSet &other) const noexcept -> bool {
    return ranges() == other.ranges();
}

auto CharSet::operator|=(const CharSet &other) -> CharSet & {
    add(other);
    return *this;
}

auto CharSet::operator&=(const CharSet &other) -> CharSet & {
    *this = intersectedWith(other);
    return *this;
}

auto CharSet::operator-=(const CharSet &other) -> CharSet & {
    remove(other);
    return *this;
}

auto CharSet::operator^=(const CharSet &other) -> CharSet & {
    *this = symmetricDifferenceWith(other);
    return *this;
}

auto CharSet::contains(const Char character) const noexcept -> bool {
    if (!character.isValidUnicode()) {
        return false;
    }
    for (const auto &range : ranges()) {
        if (range.from() > character) {
            return false;
        }
        if (range.contains(character)) {
            return true;
        }
    }
    return false;
}

auto CharSet::isSubsetOf(const CharSet &other) const -> bool {
    return subtractedBy(other).isEmpty();
}

void CharSet::add(const CharSet &other) {
    if (other.isEmpty()) {
        return;
    }
    auto newRanges = ranges();
    for (const auto &range : other.ranges()) {
        addTo(newRanges, range);
    }
    assign(std::move(newRanges));
}

void CharSet::add(const CharRange range) {
    if (range.isEmpty()) {
        return;
    }
    auto newRanges = ranges();
    addTo(newRanges, range);
    assign(std::move(newRanges));
}

void CharSet::add(const Char character) {
    add(CharRange{character});
}

void CharSet::remove(const CharSet &other) {
    if (isEmpty() || other.isEmpty()) {
        return;
    }
    auto newRanges = ranges();
    for (const auto &range : other.ranges()) {
        removeFrom(newRanges, range);
    }
    assign(std::move(newRanges));
}

void CharSet::remove(const CharRange range) {
    if (isEmpty() || range.isEmpty()) {
        return;
    }
    auto newRanges = ranges();
    removeFrom(newRanges, range);
    assign(std::move(newRanges));
}

void CharSet::remove(const Char character) {
    remove(CharRange{character});
}

auto CharSet::unitedWith(const CharSet &other) const -> CharSet {
    auto result = *this;
    result.add(other);
    return result;
}

auto CharSet::intersectedWith(const CharSet &other) const -> CharSet {
    auto resultRanges = Ranges{};
    for (const auto &left : ranges()) {
        for (const auto &right : other.ranges()) {
            if (right.to() < left.from()) {
                continue;
            }
            if (right.from() > left.to()) {
                break;
            }
            if (left.overlaps(right)) {
                addTo(resultRanges, CharRange{std::max(left.from(), right.from()), std::min(left.to(), right.to())});
            }
        }
    }
    auto result = CharSet{};
    result.assign(std::move(resultRanges));
    return result;
}

auto CharSet::subtractedBy(const CharSet &other) const -> CharSet {
    auto result = *this;
    result.remove(other);
    return result;
}

auto CharSet::symmetricDifferenceWith(const CharSet &other) const -> CharSet {
    return unitedWith(other).subtractedBy(intersectedWith(other));
}

auto CharSet::fromRange(const Char from, const Char to) -> CharSet {
    auto result = CharSet{};
    result.add(CharRange{from, to});
    return result;
}

auto CharSet::from(const AsciiCategory category) -> CharSet {
    auto result = CharSet{};
    for (auto codePoint = char32_t{0}; codePoint <= 0x7FU; ++codePoint) {
        const auto character = Char{codePoint};
        if (character.isAsciiCategory(category)) {
            result.add(character);
        }
    }
    return result;
}

auto CharSet::toString() const -> String {
    return toU8String();
}

auto CharSet::toU8String() const -> U8String {
    auto builder = StringBuilder{};
    forEach([&builder](const Char character) -> void { builder.append(character); });
    return builder.takeU8String();
}

auto CharSet::toU16String() const -> U16String {
    auto builder = StringBuilder{StringKind::U16};
    forEach([&builder](const Char character) -> void { builder.append(character); });
    return builder.takeU16String();
}

auto CharSet::toU32String() const -> U32String {
    auto builder = StringBuilder{StringKind::U32};
    forEach([&builder](const Char character) -> void { builder.append(character); });
    return builder.takeU32String();
}

auto CharSet::toSet() const -> util::Set<Char> {
    auto result = util::Set<Char>{};
    forEach([&result](const Char character) -> void { result.insert(character); });
    return result;
}

auto CharSet::toList() const -> util::List<Char> {
    auto result = util::List<Char>{};
    forEach([&result](const Char character) -> void { result.append(character); });
    return result;
}

auto CharSet::fromPattern(const U8StringView &pattern) -> CharSet {
    auto reader = StringCharReader{pattern};
    return impl::charSetFromPatternCharacters(reader);
}

auto CharSet::fromPattern(const U16StringView &pattern) -> CharSet {
    auto reader = StringCharReader{pattern};
    return impl::charSetFromPatternCharacters(reader);
}

auto CharSet::fromPattern(const U32StringView &pattern) -> CharSet {
    auto reader = StringCharReader{pattern};
    return impl::charSetFromPatternCharacters(reader);
}

void CharSet::assign(Ranges newRanges) {
    _ranges.setData(std::move(newRanges));
}

void CharSet::addTo(Ranges &ranges, CharRange range) {
    if (range.isEmpty()) {
        return;
    }

    for (auto it = ranges.begin(); it != ranges.end();) {
        if (range.canMergeWith(*it)) {
            range = range.mergedWith(*it);
            it = ranges.erase(it);
            continue;
        }
        if (range.to() < it->from()) {
            break;
        }
        ++it;
    }
    ranges.insert(range);
}

void CharSet::removeFrom(Ranges &ranges, const CharRange range) {
    if (range.isEmpty()) {
        return;
    }

    auto result = Ranges{};
    for (const auto &existing : ranges) {
        if (!existing.overlaps(range)) {
            addTo(result, existing);
            continue;
        }
        if (existing.from() < range.from()) {
            if (const auto leftEnd = previousScalar(range.from()); leftEnd.has_value() && existing.from() <= *leftEnd) {
                addTo(result, CharRange{existing.from(), *leftEnd});
            }
        }
        if (existing.to() > range.to()) {
            if (const auto rightStart = nextScalar(range.to());
                rightStart.has_value() && *rightStart <= existing.to()) {
                addTo(result, CharRange{*rightStart, existing.to()});
            }
        }
    }
    ranges = std::move(result);
}

auto CharSet::nextScalar(const Char character) noexcept -> std::optional<Char> {
    if (!character.isValidUnicode() || character.toRawValue() == 0x10FFFFU) {
        return {};
    }
    if (character.toRawValue() == 0xD7FFU) {
        return Char{0xE000U};
    }
    return Char{static_cast<char32_t>(character.toRawValue() + 1U)};
}

auto CharSet::previousScalar(const Char character) noexcept -> std::optional<Char> {
    if (!character.isValidUnicode() || character.toRawValue() == 0U) {
        return {};
    }
    if (character.toRawValue() == 0xE000U) {
        return Char{0xD7FFU};
    }
    return Char{static_cast<char32_t>(character.toRawValue() - 1U)};
}

}
