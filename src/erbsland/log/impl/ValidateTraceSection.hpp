// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../err/ParameterFailure.hpp"
#include "../../text/Char.hpp"
#include "../../text/impl/UnsafeU8StringLiteralAccess.hpp"
#include "../../text/StringLiteral.hpp"

#include <cstddef>

namespace erbsland::log::impl {

/// The maximum length of an ASCII trace-section identifier.
inline constexpr auto cMaximumTraceSectionNameLength = std::size_t{100};

/// Validate a trace-section literal during constant evaluation or at runtime.
/// @param name An ASCII configuration identifier of at most 100 characters, starting with a letter and using letters,
/// digits, `_`, or `-`; empty selects no section.
/// @tested{LogTraceSectionTest}
constexpr void validateTraceSectionName(const text::StringLiteral &name) {
    // We work on the raw data to allow a constexpr validation.
    const auto bytes = text::impl::UnsafeU8StringLiteralAccess{name}.constexprDataSpan();
    if (bytes.size() > cMaximumTraceSectionNameLength) {
        throw err::ParameterFailure{text::StringLiteral{"Invalid log trace section."}, text::StringLiteral{"value"}};
    }
    for (auto index = std::size_t{}; index < bytes.size(); ++index) {
        // As we only allow ASCII characters, reading the raw bytes is safe, as all UTF-8 characters
        // are outside the valid ASCII range. Invalid encodings also require at least one >=0x80 byte.
        const auto character = text::Char{static_cast<char32_t>(static_cast<unsigned char>(bytes[index]))};
        if (!character.isAsciiCategory(text::AsciiCategory::WordWithHyphen) ||
            (index == 0U && !character.isAsciiLetter())) {
            throw err::ParameterFailure{
                text::StringLiteral{"Invalid log trace section."}, text::StringLiteral{"value"}};
        }
    }
}

}
