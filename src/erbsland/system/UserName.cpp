// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "UserName.hpp"

#include "../text/Literals.hpp"
#include "../unit/ByteIndex.hpp"

namespace erbsland::system {

using namespace text::literals;

auto UserName::toString() const -> text::String {
    auto result = text::String{};
    if (!_domain.isEmpty()) {
        result.append(_domain);
        result.append("\\"_el);
    }
    result.append(_name);
    return result;
}

auto UserName::fromString(const text::StringView &text) -> UserName {
    constexpr auto cSeparator = "\\"_el;
    const auto separator = text.find(cSeparator);
    if (!separator.isNoIndex()) {
        const auto [domain, nameWithSeparator] = text.splitAt(separator);
        return UserName{
            nameWithSeparator.slice(text::StringSide::Back, unit::ByteIndex::end(cSeparator.length())), domain};
    }
    return UserName{text};
}

auto UserName::hash() const noexcept -> std::size_t {
    return _name.toHash() ^ (_domain.toHash() << 1U);
}

}
