// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ConfKey.hpp"

#include "../../../text/StringCharReader.hpp"
#include "../../../util/HashHelper.hpp"

namespace erbsland::conf::impl {

using namespace text;
using namespace text::literals;

auto ConfKey::isEqual(const ConfKey &other, const CaseSensitivity caseSensitivity) const noexcept -> bool {
    return _elements.compare(other._elements, caseSensitivity.asciiComparisonFn()) == std::strong_ordering::equal;
}

auto ConfKey::isEqual(
    const ConfKey &other, const CaseSensitivity caseSensitivity, const std::size_t index) const noexcept -> bool {

    return element(index).compare(other.element(index), caseSensitivity.asciiComparisonFn()) ==
        std::strong_ordering::equal;
}

auto ConfKey::elements() const noexcept -> const StringList & {
    return _elements;
}

auto ConfKey::element(const std::size_t index) const noexcept -> String {
    if (index >= _elements.count().toSizeT()) {
        return {};
    }
    return _elements.getRefOrThrow(unit::ItemIndex::fromSizeT(index));
}

auto ConfKey::toText() const noexcept -> String {
    return _elements.join(","_el);
}

auto ConfKey::size() const noexcept -> std::size_t {
    return _elements.count().toSizeT();
}

auto ConfKey::hash(const CaseSensitivity caseSensitivity) const noexcept -> std::size_t {
    std::size_t hash = 0;
    if (caseSensitivity == CaseSensitivity::CaseSensitive) {
        for (const auto &element : _elements) {
            util::advanceHash(hash, std::hash<String>{}(element));
        }
    } else {
        for (const auto &element : _elements) {
            auto reader = StringCharReader{element};
            for (auto character = reader.read(); character != Char::endOfData(); character = reader.read()) {
                util::advanceHash(hash, std::hash<char32_t>{}(character.toAsciiLowercase().toRawValue()));
            }
        }
    }
    return hash;
}

auto ConfKey::elementHash(const String &element, const CaseSensitivity caseSensitivity) noexcept -> std::size_t {
    if (caseSensitivity == CaseSensitivity::CaseSensitive) {
        return std::hash<String>{}(element);
    }
    std::size_t hash = 0;
    auto reader = StringCharReader{element};
    for (auto character = reader.read(); character != Char::endOfData(); character = reader.read()) {
        util::advanceHash(hash, std::hash<char32_t>{}(character.toAsciiLowercase().toRawValue()));
    }
    return hash;
}

}
