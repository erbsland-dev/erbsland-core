// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../text/String.hpp"

#include <cstdint>

namespace erbsland::conf {

/// The type of name.
enum class NameType : uint8_t {
    /// A regular name: name
    Regular,
    /// A text name: "text"
    Text,
    /// An index name: [&lt;index&gt;]
    Index,
    /// A text index name: ""[&lt;index&gt;]
    TextIndex,
};

[[nodiscard]] inline auto toString(const NameType nameType) noexcept -> text::String {
    using namespace text::literals;
    switch (nameType) {
    case NameType::Regular:
        return "Regular"_el;
    case NameType::Text:
        return "Text"_el;
    case NameType::Index:
        return "Index"_el;
    case NameType::TextIndex:
        return "TextIndex"_el;
    }
    return {};
}

}
