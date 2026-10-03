// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "UserName.hpp"

#include "../text/Literals.hpp"
#include "../text/StringJoins.hpp"

namespace erbsland::system {

using namespace text::literals;

auto UserName::toString() const -> text::String {
    if (_domain.isEmpty()) {
        return _name;
    }
    return text::join(_domain, "\\"_el, _name);
}

auto UserName::fromString(const text::String &text) -> UserName {
    if (const auto parts = text.splitAtFirst("\\"_el)) {
        const auto &[domain, name] = *parts;
        return UserName{name, domain};
    }
    return UserName{text};
}

auto UserName::hash() const noexcept -> std::size_t {
    return _name.toHash() ^ (_domain.toHash() << 1U);
}

}
