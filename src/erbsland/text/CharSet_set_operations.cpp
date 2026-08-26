// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CharSet.hpp"

#include "impl/CharSetFromPattern.hpp"
#include "impl/CharSetRangeBuilder.hpp"
#include "impl/CharSetRangeCounter.hpp"
#include "impl/ThrowHelper.hpp"
#include "u16/U16String.hpp"
#include "u16/U16StringEditor.hpp"
#include "u32/U32String.hpp"
#include "u32/U32StringEditor.hpp"
#include "u8/U8String.hpp"
#include "u8/U8StringEditor.hpp"

#include "../mem/impl/SharedArrayCapacity.hpp"
#include "../mem/SharedArrayData.hpp"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <limits>

namespace erbsland::text {

auto CharSet::symmetricDifferenceWith(const CharSet &other) const -> CharSet {
    const auto left = rangeSpan();
    const auto right = other.rangeSpan();
    auto builder = impl::CharSetRangeBuilder{left.size() + right.size()};
    auto leftIndex = std::size_t{0};
    auto rightIndex = std::size_t{0};
    auto leftRange = leftIndex < left.size() ? std::optional<CharRange>{left[leftIndex++]} : std::nullopt;
    auto rightRange = rightIndex < right.size() ? std::optional<CharRange>{right[rightIndex++]} : std::nullopt;

    while (leftRange.has_value() || rightRange.has_value()) {
        if (!rightRange.has_value()) {
            builder.add(*leftRange);
            leftRange = leftIndex < left.size() ? std::optional<CharRange>{left[leftIndex++]} : std::nullopt;
            continue;
        }
        if (!leftRange.has_value()) {
            builder.add(*rightRange);
            rightRange = rightIndex < right.size() ? std::optional<CharRange>{right[rightIndex++]} : std::nullopt;
            continue;
        }
        if (leftRange->to() < rightRange->from()) {
            builder.add(*leftRange);
            leftRange = leftIndex < left.size() ? std::optional<CharRange>{left[leftIndex++]} : std::nullopt;
            continue;
        }
        if (rightRange->to() < leftRange->from()) {
            builder.add(*rightRange);
            rightRange = rightIndex < right.size() ? std::optional<CharRange>{right[rightIndex++]} : std::nullopt;
            continue;
        }
        if (leftRange->from() < rightRange->from()) {
            if (const auto end = previousScalar(rightRange->from()); end.has_value()) {
                builder.add(CharRange{leftRange->from(), *end});
            }
        } else if (rightRange->from() < leftRange->from()) {
            if (const auto end = previousScalar(leftRange->from()); end.has_value()) {
                builder.add(CharRange{rightRange->from(), *end});
            }
        }
        if (leftRange->to() < rightRange->to()) {
            const auto start = nextScalar(leftRange->to());
            rightRange =
                start.has_value() ? std::optional<CharRange>{CharRange{*start, rightRange->to()}} : std::nullopt;
            leftRange = leftIndex < left.size() ? std::optional<CharRange>{left[leftIndex++]} : std::nullopt;
        } else if (rightRange->to() < leftRange->to()) {
            const auto start = nextScalar(rightRange->to());
            leftRange = start.has_value() ? std::optional<CharRange>{CharRange{*start, leftRange->to()}} : std::nullopt;
            rightRange = rightIndex < right.size() ? std::optional<CharRange>{right[rightIndex++]} : std::nullopt;
        } else {
            leftRange = leftIndex < left.size() ? std::optional<CharRange>{left[leftIndex++]} : std::nullopt;
            rightRange = rightIndex < right.size() ? std::optional<CharRange>{right[rightIndex++]} : std::nullopt;
        }
    }
    return builder.take();
}

auto CharSet::fromRange(const Char from, const Char to) -> CharSet {
    auto builder = impl::CharSetRangeBuilder{1U};
    builder.add(CharRange{from, to});
    return builder.take();
}

auto CharSet::from(const AsciiCategory category) -> CharSet {
    auto counter = impl::CharSetRangeCounter{};
    for (auto codePoint = char32_t{0}; codePoint <= 0x7FU; ++codePoint) {
        const auto character = Char{codePoint};
        if (character.isAsciiCategory(category)) {
            counter.add(CharRange{character});
        }
    }
    auto builder = impl::CharSetRangeBuilder{counter.count()};
    for (auto codePoint = char32_t{0}; codePoint <= 0x7FU; ++codePoint) {
        const auto character = Char{codePoint};
        if (character.isAsciiCategory(category)) {
            builder.add(CharRange{character});
        }
    }
    return builder.take();
}

auto CharSet::toString() const -> String {
    return toU8String();
}

auto CharSet::toU8String() const -> U8String {
    auto result = U8StringEditor{};
    forEach([&result](const Char character) -> void { result.append(character); });
    return result;
}

auto CharSet::toU16String() const -> U16String {
    auto result = U16StringEditor{};
    forEach([&result](const Char character) -> void { result.append(character); });
    return result;
}

auto CharSet::toU32String() const -> U32String {
    auto result = U32StringEditor{};
    forEach([&result](const Char character) -> void { result.append(character); });
    return result;
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

auto CharSet::fromPattern(const U8String &pattern) -> CharSet {
    auto reader = StringCharReader{pattern};
    return impl::charSetFromPatternCharacters(reader);
}

auto CharSet::fromPattern(const U16String &pattern) -> CharSet {
    auto reader = StringCharReader{pattern};
    return impl::charSetFromPatternCharacters(reader);
}

auto CharSet::fromPattern(const U32String &pattern) -> CharSet {
    auto reader = StringCharReader{pattern};
    return impl::charSetFromPatternCharacters(reader);
}

auto CharSet::rangeSpan() const noexcept -> std::span<const CharRange> {
    if (const auto *inlineRanges = std::get_if<InlineRanges>(&_storage)) {
        return std::span{inlineRanges->values.data(), inlineRangeCount(*inlineRanges)};
    }
    const auto &sharedRanges = std::get<RangeDataPtr>(_storage);
    if (sharedRanges.isNull()) {
        return {};
    }
    return std::span{sharedRanges.constGet()->data(), static_cast<std::size_t>(sharedRanges.constGet()->size())};
}

auto CharSet::inlineRangeCount(const InlineRanges &ranges) noexcept -> std::size_t {
    if (ranges.values[0].isEmpty()) {
        return 0U;
    }
    return ranges.values[1].isEmpty() ? 1U : 2U;
}

void CharSet::ensureSharedCapacity(const std::size_t requiredCapacity) {
    if (!RangeData::canAllocateWithCapacity(requiredCapacity)) {
        impl::throwOverflow("Character set range count exceeds supported bounds");
    }
    if (auto *inlineRanges = std::get_if<InlineRanges>(&_storage)) {
        const auto usedSize = inlineRangeCount(*inlineRanges);
        auto replacement = RangeDataPtr{RangeData::create(
            static_cast<RangeData::SizeType>(usedSize), static_cast<RangeData::SizeType>(requiredCapacity))};
        std::memcpy(replacement.get()->data(), inlineRanges->values.data(), usedSize * sizeof(CharRange));
        _storage = std::move(replacement);
        return;
    }
    auto &sharedRanges = std::get<RangeDataPtr>(_storage);
    const auto usedSize = static_cast<std::size_t>(sharedRanges.constGet()->size());
    mem::impl::ensureSharedArrayCapacity(
        sharedRanges, usedSize, requiredCapacity, false, [usedSize](const auto *oldData, auto *newData) -> void {
            std::memcpy(newData->data(), oldData->data(), usedSize * sizeof(CharRange));
        });
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
