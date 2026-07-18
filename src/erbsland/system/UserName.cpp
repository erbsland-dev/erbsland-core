// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "UserName.hpp"

#include "../text/Literals.hpp"
#include "../unit/ByteIndex.hpp"

namespace erbsland::system {

using namespace text::literals;

auto UserName::toString() const -> text::String {
    if (_domain.isEmpty()) {
        return _name;
    }
    return text::String::fromJoined({_domain, "\\"_el, _name});
}

auto UserName::fromString(const text::String &text) -> UserName {
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
