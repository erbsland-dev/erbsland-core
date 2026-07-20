// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EscapeFormatter.hpp"

#include "ConfigEscapeFormatter.hpp"
#include "CppEscapeFormatter.hpp"
#include "DisplayEscapeFormatter.hpp"
#include "HtmlEscapeFormatter.hpp"
#include "JsonEscapeFormatter.hpp"
#include "NoneEscapeFormatter.hpp"
#include "RegExEscapeFormatter.hpp"
#include "XmlEscapeFormatter.hpp"

#include <array>

namespace erbsland::text::impl {

auto EscapeFormatter::needsEscapeByAmount(
    const Char character, const EscapeAmount amount, const bool required, const bool balanced) noexcept -> bool {
    switch (amount.toRawValue()) {
    case EscapeAmount::Nothing:
        return false;
    case EscapeAmount::Required:
        return required;
    case EscapeAmount::Balanced:
        return required || balanced || character.isControlOrFormat();
    case EscapeAmount::NonAscii:
        return required || balanced || character.isControlOrFormat() || !character.isAscii();
    case EscapeAmount::Everything:
        return true;
    }
    return false;
}

auto EscapeFormatter::forFormat(const EscapeFormat format) noexcept -> const EscapeFormatterPtr & {
    using FormatterInstance = auto (*)() noexcept -> const EscapeFormatterPtr &;
    struct FormatterEntry {
        EscapeFormat format;
        FormatterInstance instance;
    };

    static constexpr auto table = std::array{
        FormatterEntry{EscapeFormat::Html, &HtmlEscapeFormatter::instance},
        FormatterEntry{EscapeFormat::Json, &JsonEscapeFormatter::instance},
        FormatterEntry{EscapeFormat::Cpp, &CppEscapeFormatter::instance},
        FormatterEntry{EscapeFormat::Xml, &XmlEscapeFormatter::instance},
        FormatterEntry{EscapeFormat::RegEx, &RegExEscapeFormatter::instance},
        FormatterEntry{EscapeFormat::Display, &DisplayEscapeFormatter::instance},
        FormatterEntry{EscapeFormat::Config, &ConfigEscapeFormatter::regularInstance},
        FormatterEntry{EscapeFormat::ConfigTest, &ConfigEscapeFormatter::testInstance},
        FormatterEntry{EscapeFormat::None, &NoneEscapeFormatter::instance},
    };

    for (const auto &entry : table) {
        if (entry.format == format) {
            return entry.instance();
        }
    }
    return NoneEscapeFormatter::instance();
}

}
