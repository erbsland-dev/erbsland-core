// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../text/Literals.hpp"
#include "../../../text/String.hpp"

#include <cstdint>

namespace erbsland::re::impl {

/// The data section.
enum class DataSection : uint8_t {
    Program,  ///< Program code.
    Sequence, ///< Character sequences.
    Class,    ///< Character classes.
};

/// Convert a diagnostic data-section name to its enum value.
[[nodiscard]] inline auto toDataSection(const text::String &section) noexcept -> DataSection {
    using namespace text::literals;
    if (section == "program"_el) {
        return DataSection::Program;
    }
    if (section == "sequence"_el) {
        return DataSection::Sequence;
    }
    if (section == "class"_el) {
        return DataSection::Class;
    }
    return DataSection::Program;
}

/// Convert a diagnostic data-section enum value to text.
[[nodiscard]] inline auto toString(const DataSection section) noexcept -> text::String {
    using namespace text::literals;
    switch (section) {
    case DataSection::Program:
        return "program"_el;
    case DataSection::Sequence:
        return "sequence"_el;
    case DataSection::Class:
        return "class"_el;
    default:
        break;
    }
    return {};
}

}
