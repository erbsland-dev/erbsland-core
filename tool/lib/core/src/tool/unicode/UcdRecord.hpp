// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/LineIndex.hpp>
#include <erbsland/StringList.hpp>

namespace tool::unicode {

/// One UCD record with an inclusive code-point range and its semicolon-separated fields.
/// @tested{UcdReaderTest}
struct UcdRecord {
    char32_t first{};        ///< First code point.
    char32_t last{};         ///< Last code point.
    el::StringList fields;   ///< Fields including the code-point field.
    el::LineIndex lineIndex; ///< Original line in the source file.
};

}
