// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../text/CharSet.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringSide.hpp"
#include "../../text/StringView.hpp"
#include "../../unit/CpLength.hpp"

#include <compare>
#include <cstddef>

namespace erbsland::options::impl {

/// Test if a dash-free token is a valid long, positional, or module name.
[[nodiscard]] inline auto isAsciiNameToken(const text::StringView &token) noexcept -> bool {
    using namespace text::literals;
    static const auto nameCharacters = text::CharSet::fromPattern("-_a-zA-Z0-9"_el);
    const auto [first, rest] = token.slice(text::StringSide::Front);
    if (!first.isAsciiLetter()) {
        return false;
    }
    if (!rest.containsOnly(nameCharacters) || rest.characterLength() > unit::CpLength{99U}) {
        return false;
    }
    return true;
}

}
