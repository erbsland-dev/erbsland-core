// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../text/AsciiCategory.hpp"
#include "../../text/String.hpp"
#include "../../text/StringSide.hpp"
#include "../../unit/CpLength.hpp"

#include <compare>
#include <cstddef>

namespace erbsland::options::impl {

/// Test if a dash-free token is a valid long, positional, or module name.
[[nodiscard]] inline auto isAsciiNameToken(const text::String &token) noexcept -> bool {
    const auto [first, rest] = token.slice(text::StringSide::Front);
    if (!first.isAsciiLetter()) {
        return false;
    }
    // After we know the text is ASCII only, comparing the length by bytes is faster.
    if (!rest.containsOnly(text::AsciiCategory::WordWithHyphen) || rest.length() > unit::ByteLength{99U}) {
        return false;
    }
    return true;
}

}
