// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../text/Literals.hpp"
#include "../../../text/StdFormatForText.hpp"
#include "../../../text/StringView.hpp"

#include <cstdint>
#include <format>

namespace erbsland::re::impl {

/// The data section.
enum class DataSection : uint8_t {
    Program,  ///< Program code.
    Sequence, ///< Character sequences.
    Class,    ///< Character classes.
};

[[nodiscard]] inline auto toDataSection(const text::StringView &section) noexcept -> DataSection {
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

[[nodiscard]] inline auto toString(const DataSection section) noexcept -> text::StringView {
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

template <>
struct std::formatter<erbsland::re::impl::DataSection> : std::formatter<erbsland::text::StringView> {
    auto format(const erbsland::re::impl::DataSection op, std::format_context &ctx) const {
        return std::formatter<erbsland::text::StringView>::format(erbsland::re::impl::toString(op), ctx);
    }
};
