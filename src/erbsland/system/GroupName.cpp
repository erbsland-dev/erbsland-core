// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "GroupName.hpp"

#include "../text/Literals.hpp"
#include "../unit/ByteIndex.hpp"

namespace erbsland::system {

using namespace text::literals;

auto GroupName::toString() const -> text::String {
    auto result = text::String{};
    if (!_domain.isEmpty()) {
        result.append(_domain);
        result.append("\\"_el);
    }
    result.append(_name);
    return result;
}

auto GroupName::fromString(const text::StringView &text) -> GroupName {
    constexpr auto cSeparator = "\\"_el;
    const auto separator = text.find(cSeparator);
    if (!separator.isNoIndex()) {
        const auto [domain, nameWithSeparator] = text.splitAt(separator);
        return GroupName{
            nameWithSeparator.slice(text::StringSide::Back, unit::ByteIndex::end(cSeparator.length())), domain};
    }
    return GroupName{text};
}

auto GroupName::hash() const noexcept -> std::size_t {
    return _name.toHash() ^ (_domain.toHash() << 1U);
}

}
