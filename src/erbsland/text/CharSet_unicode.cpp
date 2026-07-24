// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CharSet.hpp"

#include "impl/CharSetRangeBuilder.hpp"
#include "impl/UnicodeData.hpp"

namespace erbsland::text {

auto CharSet::isEqualToCI(const CharSet &other) const -> bool {
    return caseFolded() == other.caseFolded();
}

auto CharSet::isSubsetOfCI(const CharSet &other) const -> bool {
    return caseFolded().isSubsetOf(other.caseFolded());
}

auto CharSet::containsCaseFoldableCharacters() const -> bool {
    auto result = false;
    forEach([&](const CharRange &range) -> util::LoopStatus {
        result = range.containsCaseFoldableCharacters();
        return result ? util::LoopStatus::Stop : util::LoopStatus::Continue;
    });
    return result;
}

auto CharSet::containsLowercaseMappableCharacters() const -> bool {
    auto result = false;
    forEach([&](const CharRange &range) -> util::LoopStatus {
        result = range.containsLowercaseMappableCharacters();
        return result ? util::LoopStatus::Stop : util::LoopStatus::Continue;
    });
    return result;
}

auto CharSet::containsUppercaseMappableCharacters() const -> bool {
    auto result = false;
    forEach([&](const CharRange &range) -> util::LoopStatus {
        result = range.containsUppercaseMappableCharacters();
        return result ? util::LoopStatus::Stop : util::LoopStatus::Continue;
    });
    return result;
}

auto CharSet::toLowercase() const -> CharSet {
    if (!containsLowercaseMappableCharacters()) {
        return *this;
    }
    return transform([](const Char character) noexcept -> Char { return character.toLowercase(); });
}

auto CharSet::toUppercase() const -> CharSet {
    if (!containsUppercaseMappableCharacters()) {
        return *this;
    }
    return transform([](const Char character) noexcept -> Char { return character.toUppercase(); });
}

auto CharSet::caseFolded() const -> CharSet {
    if (!containsCaseFoldableCharacters()) {
        return *this;
    }
    return transform([](const Char character) noexcept -> Char { return character.caseFolded(); });
}

auto CharSet::from(const UnicodeCategory category) -> CharSet {
    const auto asciiData = impl::asciiUnicodeDataTable();
    const auto unicodeData = impl::unicodeDataMap();
    auto counter = impl::CharSetRangeCounter{};
    for (const auto &data : asciiData) {
        if (data.category() == category) {
            counter.add(CharRange{Char{data.start}});
        }
    }
    for (auto index = std::size_t{0}; index < unicodeData.size(); ++index) {
        const auto &data = unicodeData[index];
        if (data.category() != category) {
            continue;
        }
        const auto nextStart = index + 1 < unicodeData.size() ? unicodeData[index + 1].start : char32_t{0x110000U};
        counter.add(CharRange{Char{data.start}, Char{static_cast<char32_t>(nextStart - 1U)}});
    }
    auto builder = impl::CharSetRangeBuilder{counter.count()};
    for (const auto &data : asciiData) {
        if (data.category() == category) {
            builder.add(CharRange{Char{data.start}});
        }
    }

    for (auto index = std::size_t{0}; index < unicodeData.size(); ++index) {
        const auto &data = unicodeData[index];
        if (data.category() != category) {
            continue;
        }
        const auto nextStart = index + 1 < unicodeData.size() ? unicodeData[index + 1].start : char32_t{0x110000U};
        builder.add(CharRange{Char{data.start}, Char{static_cast<char32_t>(nextStart - 1U)}});
    }
    return builder.take();
}

auto CharSet::from(const UnicodeCategoryGroup categoryGroup) -> CharSet {
    const auto asciiData = impl::asciiUnicodeDataTable();
    const auto unicodeData = impl::unicodeDataMap();
    auto counter = impl::CharSetRangeCounter{};
    for (const auto &data : asciiData) {
        if (data.categoryGroup() == categoryGroup) {
            counter.add(CharRange{Char{data.start}});
        }
    }
    for (auto index = std::size_t{0}; index < unicodeData.size(); ++index) {
        const auto &data = unicodeData[index];
        if (data.categoryGroup() != categoryGroup) {
            continue;
        }
        const auto nextStart = index + 1 < unicodeData.size() ? unicodeData[index + 1].start : char32_t{0x110000U};
        counter.add(CharRange{Char{data.start}, Char{static_cast<char32_t>(nextStart - 1U)}});
    }
    auto builder = impl::CharSetRangeBuilder{counter.count()};
    for (const auto &data : asciiData) {
        if (data.categoryGroup() == categoryGroup) {
            builder.add(CharRange{Char{data.start}});
        }
    }

    for (auto index = std::size_t{0}; index < unicodeData.size(); ++index) {
        const auto &data = unicodeData[index];
        if (data.categoryGroup() != categoryGroup) {
            continue;
        }
        const auto nextStart = index + 1 < unicodeData.size() ? unicodeData[index + 1].start : char32_t{0x110000U};
        builder.add(CharRange{Char{data.start}, Char{static_cast<char32_t>(nextStart - 1U)}});
    }
    return builder.take();
}

}
